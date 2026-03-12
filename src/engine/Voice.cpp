#include "Voice.h"
#include "../PluginProcessor.h"

Voice::Voice (TetraOPAudioProcessor& p, int _id)
    : audioProcessor (p)
	, id(_id)
    , batch(_id / SIMDSZ)
    , lane(_id % SIMDSZ)
{
    osc.reserve(4);
    for (int i = 0; i < MAX_OSCILLATORS; ++i)
    {
        osc.emplace_back(i, _id, audioProcessor);
    }
}

void Voice::noteStarted()
{
    audioProcessor.modulation->lastUsedVoice = id;
    fastKill = false;

    pressed = true;
    released = false;
    attack_elapsed = 0.f;
    release_elapsed = 0.f;

    pressed_ts = pressed_ts_counter;
    pressed_ts_counter += 1;

    auto note = getCurrentlyPlayingNote();
    vel = audioProcessor.modulation->velCurve.get_y_at(note.noteOnVelocity.asUnsignedFloat());
    vel_targ = vel;
    key = note.initialNote / 127.f;
    mpe_channel = note.midiChannel;

    if (glideInfo.fromNote >= 0 && glideInfo.portamento)
    {
        noteSmoother.setTime (glideInfo.rate);
        noteSmoother.setValueUnsmoothed (glideInfo.fromNote / 127.0f);
        noteSmoother.setValue (note.initialNote / 127.0f);
    }
    else
    {
        noteSmoother.setValueUnsmoothed (note.initialNote / 127.0f);
    }

    for (int i = 0; i < MAX_OSCILLATORS; i++)
    {
        osc[i].trigger(note.initialNote, srate);
    }

    updateFilters(true);
    auto& voice = audioProcessor.synth->vox[batch].voice;
    voice.key[lane] = note.initialNote;
}

void Voice::noteRetriggered()
{
    audioProcessor.modulation->lastUsedVoice = id;
    auto& envmod = audioProcessor.modulation->envs[0];
    if (released && envmod.mode != Envelope::AHD)
    {
        attack_elapsed = envmod.getMatchingAttackTimeFromRelease(attack_elapsed, release_elapsed);
        release_elapsed = 0.f;
        released = false;
    }

    auto note = getCurrentlyPlayingNote();

    // in mono mode retain velocity during retriggers
    if (!audioProcessor.synth->lastEventWasNoteOff && !released)
    {
        vel_targ = audioProcessor.modulation->velCurve.get_y_at(note.noteOnVelocity.asUnsignedFloat());
        vel_step = (vel_targ - vel) / float(getSampleRate() * 0.005); // 5 ms interpolation to avoid clicks
    }

    key = note.initialNote / 127.f;

    if (glideInfo.fromNote >= 0 && glideInfo.portamento)
    {
        noteSmoother.setTime (glideInfo.rate);
        noteSmoother.setValue (key);
    }
    else
    {
        noteSmoother.setValueUnsmoothed (key);
    }

    auto& voice = audioProcessor.synth->vox[batch].voice;
    voice.key[lane] = note.initialNote;
}

void Voice::noteStopped (bool allowTailOff)
{
    released = true;
    pressed = false;

    juce::Array<Voice*> activeVoices;
    for (auto voice : audioProcessor.synth->getActiveVoices())
        if (voice->isActive() && ((Voice*)voice)->pressed)
            activeVoices.add((Voice*)voice);
    std::sort(activeVoices.begin(), activeVoices.end(), [](Voice* a, Voice* b) { return a->noteOnTime < b->noteOnTime; });

    auto lastVoice = activeVoices.getLast();
    if (lastVoice)
        audioProcessor.modulation->lastUsedVoice = lastVoice->id;

    if (! allowTailOff)
    {
        clearCurrentNote();
    }
}

void Voice::notePressureChanged()
{
    //auto note = getCurrentlyPlayingNote();
    //proc.modMatrix.setPolyValue (*this, proc.modSrcPressure, note.pressure.asUnsignedFloat());
}

void Voice::noteTimbreChanged()
{
    //auto note = getCurrentlyPlayingNote();
    //proc.modMatrix.setPolyValue (*this, proc.modSrcTimbre, note.initialTimbre.asUnsignedFloat());
}

void Voice::setCurrentSampleRate (double newRate)
{
    MPESynthesiserVoice::setCurrentSampleRate (newRate);
    noteSmoother.setSampleRate (newRate);
    srate = (float)getSampleRate();
    israte = 1.f / srate;
}

void Voice::startBlock(int startSample, int numSamples)
{
    auto& voice = audioProcessor.synth->vox[batch].voice;
    bool msk[4] = { false, false, false, false };
    msk[lane] = true;
    SIMDM mask = SIMDM(msk);

    int blkoffset = startSample - audioProcessor.currBlockPos + numSamples;

    auto env_targ = audioProcessor.modulation->getEnvelopeValue(0, id, blkoffset);
    if (fastKill)
        env_targ *= 0.01f; // TODO use a proper fadeout
    Utils::setMasked(voice.env_step, (env_targ - voice.env.get(lane)) / numSamples, mask);
    Utils::setMasked(voice.vel_mult, vel * audioProcessor.velsense + 1.0f - audioProcessor.velsense, mask);

    updateFilters(false, blkoffset);
}

void Voice::endBlock(int startSample, int numSamples)
{
    (void)startSample;
    noteSmoother.process(numSamples);

    if (released)
        release_elapsed += (float)(numSamples * israte);
    else
        attack_elapsed += (float)(numSamples * israte);

    // check if envelope has finished
    if (released && release_elapsed > audioProcessor.modulation->envs[0].rel)
    {
        clearCurrentNote();
    }
}

void Voice::updateFilters(bool init, int blkoffset)
{
    auto f1_on = (bool)audioProcessor.params.getRawParameterValue("f1_on")->load();
    if (init || f1_on) 
    {
        auto f1_cut = audioProcessor.modulation->getPolyValue("f1_cut", id, blkoffset);
        auto f1_res = audioProcessor.modulation->getPolyValue("f1_res", id, blkoffset);
        auto f1_drive = audioProcessor.modulation->getPolyValue("f1_drive", id, blkoffset);
        auto f1_mix = audioProcessor.modulation->getPolyValue("f1_mix", id, blkoffset);
        if (init)
            audioProcessor.synth->initFilters(id, 0, f1_cut, f1_res, f1_drive, f1_mix);
        else
            audioProcessor.synth->updateFilters(id, 0, f1_cut, f1_res, f1_drive, f1_mix);
    }

    auto f2_on = (bool)audioProcessor.params.getRawParameterValue("f2_on")->load();
    if (init || f2_on)
    {
        auto f2_cut = audioProcessor.modulation->getPolyValue("f2_cut", id, blkoffset);
        auto f2_res = audioProcessor.modulation->getPolyValue("f2_res", id, blkoffset);
        auto f2_drive = audioProcessor.modulation->getPolyValue("f2_drive", id, blkoffset);
        auto f2_mix = audioProcessor.modulation->getPolyValue("f2_mix", id, blkoffset);
        if (init)
            audioProcessor.synth->initFilters(id, 1, f2_cut, f2_res, f2_drive, f2_mix);
        else
            audioProcessor.synth->updateFilters(id, 1, f2_cut, f2_res, f2_drive, f2_mix);
    }
}
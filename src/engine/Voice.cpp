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
    vel_step = 0.f;
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

    if (audioProcessor.synth->fm->layout == FmMatrix::Layout::Custom 
        && audioProcessor.modulation->isFmMatrixModulated())
    {
        updateMatrix(voice);
    }

    // FIX pops by resetting envelope to zero
    bool msk[4] = { false, false, false, false };
    msk[lane] = true;
    SIMDM mask = SIMDM(msk);
    Utils::setMasked(voice.env, 0.f, mask);

    auto velsense = audioProcessor.params.getRawParameterValue("vel_sense")->load();
    Utils::setMasked(voice.vel_mult, vel * velsense + 1.0f - velsense, mask);
    Utils::setMasked(voice.vel_step, 0.f, mask);
}

void Voice::noteRetriggered()
{
    bool msk[4] = { false, false, false, false };
    msk[lane] = true;
    SIMDM mask = SIMDM(msk);

    audioProcessor.modulation->lastUsedVoice = id;
    auto& envmod = audioProcessor.modulation->envs[0];

    pressed = true;
    pressed_ts = pressed_ts_counter;
    pressed_ts_counter += 1;

    if (released && envmod.mode != Envelope::AHD)
    {
        attack_elapsed = envmod.getMatchingAttackTimeFromRelease(attack_elapsed, release_elapsed);
        release_elapsed = 0.f;
        released = false;
    }

    bool legato = (bool)audioProcessor.params.getRawParameterValue("legato")->load();
    if (!legato)
    {
        attack_elapsed = 0;
        release_elapsed = 0;
        released = false;
    }

    auto note = getCurrentlyPlayingNote();

    // hammering a new note with others pressed in MONO
    // set current velocity to the new velocity
    // if its hammerOff retain velocity
    bool isHammerOn = !audioProcessor.synth->lastEventWasNoteOff && !released;
    if (isHammerOn)
    {
        vel_targ = audioProcessor.modulation->velCurve.get_y_at(note.noteOnVelocity.asUnsignedFloat());
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

    for (int i = 0; i < MAX_OSCILLATORS; i++)
    {
        osc[i].retrigger(note.initialNote, srate);
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

    if (vel != vel_targ)
    {
        float velsense = audioProcessor.params.getRawParameterValue("vel_sense")->load();
        float vcurr = vel * velsense + 1.0f - velsense;
        float vtarg = vel_targ * velsense + 1.0f - velsense;
        Utils::setMasked(voice.vel_step, (vtarg - vcurr) / numSamples, mask);
    }

    updateFilters(false, blkoffset);
    if (audioProcessor.synth->fm->layout == FmMatrix::Layout::Custom 
        && audioProcessor.modulation->isFmMatrixModulated())
    {
        updateMatrix(voice, blkoffset);
    }
}

void Voice::endBlock(int, int numSamples)
{
    noteSmoother.process(numSamples);

    if (released)
        release_elapsed += (float)(numSamples * israte);
    else
        attack_elapsed += (float)(numSamples * israte);

    // velocity is interpolated over a block in MONO mode
    if (vel != vel_targ) 
    {
        auto& voice = audioProcessor.synth->vox[batch].voice;
        bool msk[4] = { false, false, false, false };
        msk[lane] = true;
        SIMDM mask = SIMDM(msk);

        vel = vel_targ;
        Utils::setMasked(voice.vel_step, 0.f, mask);
    }

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

/*
* Expensive matrix update per voice per block so that the matrix is fully modulatable
*/
void Voice::updateMatrix(SIMDVoice& voice, int blkoffset)
{
    bool msk[4] = { false, false, false, false };
    msk[lane] = true;
    SIMDM mask = SIMDM(msk);

    auto& mod = audioProcessor.modulation;
    Utils::setMasked(voice.fm_ab, mod->getPolyValue("fm_ab", id, blkoffset), mask);
    Utils::setMasked(voice.fm_ac, mod->getPolyValue("fm_ac", id, blkoffset), mask);
    Utils::setMasked(voice.fm_ad, mod->getPolyValue("fm_ad", id, blkoffset), mask);

    Utils::setMasked(voice.fm_ba, mod->getPolyValue("fm_ba", id, blkoffset), mask);
    Utils::setMasked(voice.fm_bc, mod->getPolyValue("fm_bc", id, blkoffset), mask);
    Utils::setMasked(voice.fm_bd, mod->getPolyValue("fm_bd", id, blkoffset), mask);

    Utils::setMasked(voice.fm_ca, mod->getPolyValue("fm_ca", id, blkoffset), mask);
    Utils::setMasked(voice.fm_cb, mod->getPolyValue("fm_cb", id, blkoffset), mask);
    Utils::setMasked(voice.fm_cd, mod->getPolyValue("fm_cd", id, blkoffset), mask);

    Utils::setMasked(voice.fm_da, mod->getPolyValue("fm_da", id, blkoffset), mask);
    Utils::setMasked(voice.fm_db, mod->getPolyValue("fm_db", id, blkoffset), mask);
    Utils::setMasked(voice.fm_dc, mod->getPolyValue("fm_dc", id, blkoffset), mask);

    Utils::setMasked(voice.fm_aout, mod->getPolyValue("fm_aout", id, blkoffset), mask);
    Utils::setMasked(voice.fm_bout, mod->getPolyValue("fm_bout", id, blkoffset), mask);
    Utils::setMasked(voice.fm_cout, mod->getPolyValue("fm_cout", id, blkoffset), mask);
    Utils::setMasked(voice.fm_dout, mod->getPolyValue("fm_dout", id, blkoffset), mask);

    Utils::setMasked(voice.rm_aa, mod->getPolyValue("rm_aa", id, blkoffset), mask);
    Utils::setMasked(voice.rm_ab, mod->getPolyValue("rm_ab", id, blkoffset), mask);
    Utils::setMasked(voice.rm_ac, mod->getPolyValue("rm_ac", id, blkoffset), mask);
    Utils::setMasked(voice.rm_ad, mod->getPolyValue("rm_ad", id, blkoffset), mask);

    Utils::setMasked(voice.rm_ba, mod->getPolyValue("rm_ba", id, blkoffset), mask);
    Utils::setMasked(voice.rm_bb, mod->getPolyValue("rm_bb", id, blkoffset), mask);
    Utils::setMasked(voice.rm_bc, mod->getPolyValue("rm_bc", id, blkoffset), mask);
    Utils::setMasked(voice.rm_bd, mod->getPolyValue("rm_bd", id, blkoffset), mask);

    Utils::setMasked(voice.rm_ca, mod->getPolyValue("rm_ca", id, blkoffset), mask);
    Utils::setMasked(voice.rm_cb, mod->getPolyValue("rm_cb", id, blkoffset), mask);
    Utils::setMasked(voice.rm_cc, mod->getPolyValue("rm_cc", id, blkoffset), mask);
    Utils::setMasked(voice.rm_cd, mod->getPolyValue("rm_cd", id, blkoffset), mask);

    Utils::setMasked(voice.rm_da, mod->getPolyValue("rm_da", id, blkoffset), mask);
    Utils::setMasked(voice.rm_db, mod->getPolyValue("rm_db", id, blkoffset), mask);
    Utils::setMasked(voice.rm_dc, mod->getPolyValue("rm_dc", id, blkoffset), mask);
    Utils::setMasked(voice.rm_dd, mod->getPolyValue("rm_dd", id, blkoffset), mask);
}
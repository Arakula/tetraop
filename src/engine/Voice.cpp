#include "Voice.h"
#include "../PluginProcessor.h"

Voice::Voice (TetraOPAudioProcessor& p, int _id)
    : audioProcessor (p)
	, id(_id)
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
}

void Voice::noteStopped (bool allowTailOff)
{
    released = true;
    pressed = false;

    auto lastVoice = (Voice*)audioProcessor.synth->getNewestVoice();
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
    (void)startSample;
    auto env_targ = audioProcessor.modulation->getEnvelopeValue(0, id, numSamples);
    if (fastKill)
        env_targ *= 0.01f; // TODO use a proper fadeout
    env_step = (env_targ - env) / numSamples;
    vel_mult = vel * audioProcessor.velsense + 1.0f - audioProcessor.velsense;
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
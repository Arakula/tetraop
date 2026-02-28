#include "Voice.h"
#include "../PluginProcessor.h"

Voice::Voice (TetraOPAudioProcessor& p, int _id)
    : audioProcessor (p)
	, id(_id)
{
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

    for (int i = 0; i < MAX_OPERATORS; i++)
    {
        osc[i].phase = 0.f;
        osc[i].freq = 440.0f * std::pow(2.0f, (note.initialNote - 69) / 12.0f);
        osc[i].phase_inc = osc[i].freq * israte;
        osc[i].level = i == 0 ? 0.5f : 0.35f;
        osc[i].out = 0.f;
        osc[i].unison_phases_inc[0] = osc[i].phase_inc * 0.9f;
        osc[i].unison_phases_inc[1] = osc[i].phase_inc * 1.1f;

        for (int j = 0; j < MAX_UNISON; j++)
        {
            //osc[i].unison_phases[j] = rand() / (float)RAND_MAX;
            osc[i].unison_phases[j] = 0.f;
            osc[i].unison_mask[j] = j < 2;
        }
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


void Voice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    gin::ScratchBuffer buffer(2, numSamples);

    auto note = getCurrentlyPlayingNote();
    float currentMidiNote = noteSmoother.getCurrentValue() * 127.0f;
    currentMidiNote += float(note.totalPitchbendInSemitones);
    //float freq = float(std::min(srate / 2.0, 440.0 * std::pow(2.0, (currentMidiNote - 69.0) / 12.0)));

    auto env_targ = audioProcessor.modulation->getEnvelopeValue(0, id, startSample + numSamples);
    //auto env_step = (env_targ - env) / numSamples;

    float velmult = vel * audioProcessor.velsense + 1.0f - audioProcessor.velsense;

    auto l = buffer.getWritePointer(0);
    auto r = buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        *l++ *= velmult * env;
        *r++ *= velmult * env;

        // interpolation
        env += env_step;
        if (vel != vel_targ)
        {
            vel += vel_step;
            if (vel_step > 0.f && vel > vel_targ || vel_step < 0.f && vel < vel_targ)
            {
                vel = vel_targ;
                vel_step = 0.f;
            }
            velmult = vel * audioProcessor.velsense + 1.0f - audioProcessor.velsense;
        }
    }

    // Copy output to synth
    outputBuffer.addFrom (0, startSample, buffer, 0, 0, numSamples);
    outputBuffer.addFrom (1, startSample, buffer, 1, 0, numSamples);

    noteSmoother.process(numSamples);
    env = env_targ;

    if (released)
        release_elapsed += (float)(numSamples / srate);
    else
        attack_elapsed += (float)(numSamples / srate);

    // check if envelope has finished
    if (released && release_elapsed > audioProcessor.modulation->envs[0].rel)
    {
        clearCurrentNote();
    }
}

void Voice::startBlock(int startSample, int numSamples)
{
    auto env_targ = audioProcessor.modulation->getEnvelopeValue(0, id, startSample + numSamples);
    if (fastKill)
        env_targ *= 0.01f; // TODO use a proper fadeout
    env_step = (env_targ - env) / numSamples;
    vel_mult = vel * audioProcessor.velsense + 1.0f - audioProcessor.velsense;
}

void Voice::endBlock(int startSample, int numSamples)
{
    // TODO env = env_targ
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
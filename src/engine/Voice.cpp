#include "Voice.h"
#include "../PluginProcessor.h"

Voice::Voice (TetraOPAudioProcessor& p, int _id)
    : audioProcessor (p)
	, id(_id)
{
}

void Voice::noteStarted()
{
    srate = (float)getSampleRate();
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
}

void Voice::noteRetriggered()
{
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
}

void Voice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    gin::ScratchBuffer buffer (2, numSamples);
    auto l = buffer.getWritePointer(0);
    auto r = buffer.getWritePointer(1);

    auto note = getCurrentlyPlayingNote();
    float currentMidiNote = noteSmoother.getCurrentValue() * 127.0f;
    currentMidiNote += float(note.totalPitchbendInSemitones);
    float freq = float(std::min(srate / 2.0, 440.0 * std::pow(2.0, (currentMidiNote - 69.0) / 12.0)));

    const float phaseInc = juce::MathConstants<float>::twoPi * freq / srate;
    
    auto env_targ = audioProcessor.modulation->getEnvelopeValue(0, id, startSample + numSamples);
    auto env_step = (env_targ - env) / numSamples;

    float velmult = vel * audioProcessor.velsense + 1.0f - audioProcessor.velsense;

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = (float)std::sin(phase);

        phase += phaseInc;
        if (phase >= juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;

        *l++ += sample * velmult * env;
        *r++ += sample * velmult * env;

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


    //buffer.applyGain(gin::velocityToGain(vel, ampKeyTrack));

    //if (adsr.getState() == gin::AnalogADSR::State::idle)
    //{
    //    clearCurrentNote();
    //    stopVoice();
    //}

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

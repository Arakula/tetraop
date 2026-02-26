#include "Voice.h"
#include "../PluginProcessor.h"

Voice::Voice (TetraOPAudioProcessor& p, int _id)
    : audioProcessor (p)
	, id(_id)
{
}

void Voice::noteStarted()
{
    fastKill = false;

    auto note = getCurrentlyPlayingNote();
    if (glideInfo.fromNote >= 0 && (glideInfo.glissando || glideInfo.portamento))
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
    auto note = getCurrentlyPlayingNote();

    if (glideInfo.fromNote >= 0 && (glideInfo.glissando || glideInfo.portamento))
    {
        noteSmoother.setTime (glideInfo.rate);
        noteSmoother.setValue (note.initialNote / 127.0f);
    }
    else
    {
        noteSmoother.setValueUnsmoothed (note.initialNote / 127.0f);
    }
}

void Voice::noteStopped (bool allowTailOff)
{
    //if (! allowTailOff)
    //{
        clearCurrentNote();
    //}
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
    float freq = float(std::min(audioProcessor.osrate / 2.0, 440.0 * std::pow(2.0, (currentMidiNote - 69.0) / 12.0)));

    const float phaseInc = juce::MathConstants<float>::twoPi * freq / audioProcessor.osrate;
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = std::sin(phase);

        phase += phaseInc;
        if (phase >= juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;

        *l++ += sample;
        *r++ += sample;
    }

    float velocity = currentlyPlayingNote.noteOnVelocity.asUnsignedFloat();
    buffer.applyGain(gin::velocityToGain(velocity, ampKeyTrack));

    //if (adsr.getState() == gin::AnalogADSR::State::idle)
    //{
    //    clearCurrentNote();
    //    stopVoice();
    //}

    // Copy output to synth
    outputBuffer.addFrom (0, startSample, buffer, 0, 0, numSamples);
    outputBuffer.addFrom (1, startSample, buffer, 1, 0, numSamples);
    noteSmoother.process(numSamples);
}

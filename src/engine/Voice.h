// Copyright 2026 tilr

#pragma once

#include <JuceHeader.h>

class TetraOPAudioProcessor;

//==============================================================================
class Voice : public gin::SynthesiserVoice
{
public:
	int id;

    Voice (TetraOPAudioProcessor& p, int id);

    float getCurrentNote() override { return noteSmoother.getCurrentValue() * 127.0f; }

    void noteStarted() override;
    void noteRetriggered() override;
    void noteStopped (bool allowTailOff) override;

    void notePressureChanged() override;
    void noteTimbreChanged() override;
    void notePitchbendChanged() override    {}
    void noteKeyStateChanged() override     {}

    void setCurrentSampleRate (double newRate) override;
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    void updateParams (int blockSize);

    TetraOPAudioProcessor& audioProcessor;

    float currentMidiNote;
    gin::EasedValueSmoother<float> noteSmoother;

    float ampKeyTrack = 1.0f;


    double phase = 0.f;
};

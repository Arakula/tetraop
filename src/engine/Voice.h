// Copyright 2026 tilr

#pragma once

#include <JuceHeader.h>
#include "FmMatrix.h"

class TetraOPAudioProcessor;

//==============================================================================
class Voice : public gin::SynthesiserVoice
{
public:
	int id;
    uint64_t pressed_ts = 1; // timestamp used for rand generators based on note start
    float srate = 44100.f;
    float attack_elapsed = 0.0f; // elapsed time in seconds
    float release_elapsed = 0.0f;
    bool released = false;
    bool pressed = false;
    float vel = 0.f; // norm velocity
    float key = 0.f; // norm note used for keytracking
    int mpe_channel = 1;
    float env = 0.f; // adsr envelope value

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
    static inline uint64_t pressed_ts_counter = 1;
    TetraOPAudioProcessor& audioProcessor;
    gin::EasedValueSmoother<float> noteSmoother;
    std::unique_ptr<FmMatrix> fm;

    float ampKeyTrack = 1.0f;
    double phase = 0.f;

    // interpolation
    float vel_targ = 0.f;
    float vel_step = 0.f;
};

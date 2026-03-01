// Copyright 2026 tilr

#pragma once

#include <JuceHeader.h>
#include "Utils.h"
#include "OSC.h"

class TetraOPAudioProcessor;

//==============================================================================
class Voice : public gin::SynthesiserVoice
{
public:

#if GIN_HAS_SIMD
    struct SIMDVoice
    {
        SIMDF env;
        SIMDF vel_mult;
        SIMDF env_step;
    };

    struct VoiceVec
    {
        alignas(sizeof(SIMDF)) float vel_mult[4];
        alignas(sizeof(SIMDF)) float env[4];
        alignas(sizeof(SIMDF)) float env_step[4];
    };

    void stateToVec(VoiceVec& vec, int lane) const
    {
        vec.env[lane] = env;
        vec.vel_mult[lane] = vel_mult;
        vec.env_step[lane] = env_step;
    }

    void vecToState(VoiceVec& vec, int lane)
    {
        env = vec.env[lane];
        vel_mult = vec.vel_mult[lane];
    }

    static SIMDVoice vecToSIMD(VoiceVec& vec)
    {
        SIMDVoice v;
        v.env.load(vec.env);
        v.vel_mult.load(vec.vel_mult);
        v.env_step.load(vec.env_step);
        return v;
    }

    static VoiceVec SIMDToVec(SIMDVoice simd)
    {
        VoiceVec vec{};
        simd.vel_mult.store(vec.vel_mult);
        simd.env.store(vec.env);
        simd.env_step.store(vec.env_step);
        return vec;
    }
#endif

	int id;
    uint64_t pressed_ts = 1; // timestamp used for rand generators based on note start
    float srate = 44100.f;
    float israte = 0.0001f;
    float attack_elapsed = 0.0f; // elapsed time in seconds
    float release_elapsed = 0.0f;
    bool released = false;
    bool pressed = false;
    float vel = 0.f; // norm velocity
    float key = 0.f; // norm note used for keytracking
    int mpe_channel = 1;
    float env = 0.f; // adsr envelope value
    float env_step = 0.f;
    float vel_mult = 1.f;

    std::vector<OSC> osc;

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
    void startBlock(int startSample, int numSamples);
    void endBlock(int startSample, int numSamples);

private:
    static inline uint64_t pressed_ts_counter = 1;
    TetraOPAudioProcessor& audioProcessor;
    gin::EasedValueSmoother<float> noteSmoother;

    float ampKeyTrack = 1.0f;
    double phase = 0.f;

    // interpolation
    float vel_targ = 0.f;
    float vel_step = 0.f;
};

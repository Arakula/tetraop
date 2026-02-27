// Copyright 2026 tilr

#pragma once

#include <JuceHeader.h>
#include "Utils.h"

class TetraOPAudioProcessor;


struct OSC
{
    struct SIMDOSC
    {
        mipp::Reg<float> phase;
        mipp::Reg<float> phase_inc;
        mipp::Reg<float> freq;
        mipp::Reg<float> level;
        mipp::Reg<float> out;
    };

    struct OSCVec
    {
        alignas(sizeof(SIMDF)) float phase[4];
        alignas(sizeof(SIMDF)) float phase_inc[4];
        alignas(sizeof(SIMDF)) float freq[4];
        alignas(sizeof(SIMDF)) float level[4];
        alignas(sizeof(SIMDF)) float out[4];
    };

    void stateToVec(OSCVec& vec, int lane) const
    {
        vec.phase[lane] = phase;
        vec.phase_inc[lane] = phase_inc;
        vec.freq[lane] = freq;
        vec.level[lane] = level;
        vec.out[lane] = out;
    }

    void vecToState(OSCVec& vec, int lane)
    {
        phase = vec.phase[lane];
        phase_inc = vec.phase_inc[lane];
        freq = vec.freq[lane];
        level = vec.level[lane];
        out = vec.out[lane];
    }

    static SIMDOSC vecToSIMD(OSCVec& vec)
    {
        SIMDOSC o;
        o.phase.load(vec.phase);
        o.phase_inc.load(vec.phase_inc);
        o.freq.load(vec.freq);
        o.level.load(vec.level);
        o.out.load(vec.out);
        return o;
    }

    static OSCVec SIMDToVec(SIMDOSC simd)
    {
        OSCVec vec{};
        simd.phase.store(vec.phase);
        simd.phase_inc.store(vec.phase_inc);
        simd.freq.store(vec.freq);
        simd.level.store(vec.level);
        simd.out.store(vec.out);
        return vec;
    }

    float phase;
    float freq;
    float phase_inc;
    float level;
    float out;
};

//==============================================================================
class Voice : public gin::SynthesiserVoice
{
public:

#if GIN_HAS_SIMD
    struct SIMDVoice
    {
        mipp::Reg<float> env;
        mipp::Reg<float> vel_mult;
        mipp::Reg<float> env_step;
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
        SIMDVoice s;
        s.env.load(vec.env);
        s.vel_mult.load(vec.vel_mult);
        s.env_step.load(vec.env_step);
        return s;
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

    OSC osc[4] = {};

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

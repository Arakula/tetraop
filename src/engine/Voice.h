// Copyright 2026 tilr

#pragma once

#include <JuceHeader.h>
#include "Utils.h"

class TetraOPAudioProcessor;

struct OSC
{
    struct SIMDUnison
    {
        int voices = 1;
        SIMDF phase[4]; // four batches for each lane == MAX 16 unison voices per osc per voice
        SIMDF inc[4];
        SIMDF mask[4];
    };

    struct UnisonVec
    {
        alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> phase{};
        alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> inc{};
        alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> gainL{};
        alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> gainR{};
        alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> mask{};
        int voices = 1;
    };

    struct SIMDOSC
    {
        SIMDF phase;
        SIMDF phase_inc;
        SIMDF freq;
        SIMDF level;
        SIMDF out;
        SIMDUnison unison[4]; // four lanes of voices
    };

    struct OSCVec
    {
        alignas(sizeof(SIMDF)) float phase[4];
        alignas(sizeof(SIMDF)) float phase_inc[4];
        alignas(sizeof(SIMDF)) float freq[4];
        alignas(sizeof(SIMDF)) float level[4];
        alignas(sizeof(SIMDF)) float out[4];
        UnisonVec unison[4];
    };

    void stateToVec(OSCVec& vec, int lane) const
    {
        vec.phase[lane] = phase;
        vec.phase_inc[lane] = phase_inc;
        vec.freq[lane] = freq;
        vec.level[lane] = level;
        vec.out[lane] = out;
        vec.unison[lane].voices = uni_voices;

        if (uni_voices > 1) 
        {
            vec.unison[lane].phase = unison_phases;
            vec.unison[lane].inc = unison_phases_inc;
            vec.unison[lane].mask = unison_mask;
        }
    }

    void vecToState(OSCVec& vec, int lane)
    {
        phase = vec.phase[lane];
        phase_inc = vec.phase_inc[lane];
        freq = vec.freq[lane];
        level = vec.level[lane];
        out = vec.out[lane];

        if (vec.unison[lane].voices > 1)
        {
            unison_phases = vec.unison[lane].phase;
            unison_phases_inc = vec.unison[lane].inc;
        }
    }

    static SIMDOSC vecToSIMD(OSCVec& vec)
    {
        SIMDOSC o;
        o.phase.load(vec.phase);
        o.phase_inc.load(vec.phase_inc);
        o.freq.load(vec.freq);
        o.level.load(vec.level);
        o.out.load(vec.out);
        
        for (int lane = 0; lane < 4; ++lane)
        {
            o.unison[lane].voices = vec.unison[lane].voices;
            if (o.unison[lane].voices == 1)
                continue;

            for (int batch = 0; batch < 4; batch++)
            {
                auto idx = batch * 4;
                o.unison[lane].phase[batch].load(&vec.unison[lane].phase[idx]);
                o.unison[lane].inc[batch].load(&vec.unison[lane].inc[idx]);
                o.unison[lane].mask[batch].load(&vec.unison[lane].mask[idx]);
            }
        }

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

        for (int lane = 0; lane < 4; ++lane)
        {
            vec.unison[lane].voices = simd.unison[lane].voices;
            if (simd.unison[lane].voices == 1)
                continue;

            for (int batch = 0; batch < 4; batch++)
            {
                simd.unison[lane].phase[batch].store(&vec.unison[lane].phase[batch * 4]);
                simd.unison[lane].inc[batch].store(&vec.unison[lane].inc[batch * 4]);
            }
        }

        return vec;
    }

    alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> unison_phases;
    alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> unison_phases_inc;
    alignas(sizeof(SIMDM)) std::array<float, MAX_UNISON> unison_mask;

    float phase;
    float freq;
    float phase_inc;
    float level;
    float out;
    int uni_voices = 2;
};

//==============================================================================
class Voice : public gin::SynthesiserVoice
{
public:

#if GIN_HAS_SIMD
    struct SIMDVoice
    {
        SIMDF mask;
        SIMDF env;
        SIMDF vel_mult;
        SIMDF env_step;
    };

    struct VoiceVec
    {
        alignas(sizeof(SIMDF)) float mask[4] = { 0.f, 0.f, 0.f, 0.f };
        alignas(sizeof(SIMDF)) float vel_mult[4];
        alignas(sizeof(SIMDF)) float env[4];
        alignas(sizeof(SIMDF)) float env_step[4];
    };

    void stateToVec(VoiceVec& vec, int lane) const
    {
        vec.mask[lane] = 1.f;
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
        s.mask.load(vec.mask);
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

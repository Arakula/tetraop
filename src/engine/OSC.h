#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "Utils.h"
#include "Unison.h"

using namespace globals;

class TetraOPAudioProcessor;

class OSC
{
public:
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
        alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> gain_l{};
        alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> gain_r{};
        alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> mask{};
        alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> ratio{};
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
        alignas(sizeof(SIMDF)) float gain_l[4];
        alignas(sizeof(SIMDF)) float gain_r[4];
        UnisonVec unison[4];
    };

    alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> unison_phase{};
    alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> unison_ratio{};
    alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> unison_mask{};
    alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> unison_gain_l{};
    alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> unison_gain_r{};

    bool isOn = false;
    int voiceId = 0;
    int id = 0;
    String prefix = "";
    float phase = 0.f;
    float freq = 1000.f;
    float phase_inc = 0.f;
    float level = 0.f;
    float out = 0.f;
    float pan = 0.f;
    float gain_l = 0.f;
    float gain_r = 0.f;
    int unison_voices = 1;
    int unison_mode = 0;
    float unison_detune = -1.f;
    float unison_stereo = -1.f;
    float unison_spread = -1.f;
    float unison_blend = -1.f;

    OSC(int _id, int _voiceId, TetraOPAudioProcessor& p);
    ~OSC() {}

    void prepareBlock(int startSample, int numSamples);
    void recalcUnison();

    void stateToVec(OSCVec& vec, int lane) const
    {
        vec.phase[lane] = phase;
        vec.phase_inc[lane] = phase_inc;
        vec.freq[lane] = freq;
        vec.level[lane] = level;
        vec.out[lane] = out;
        vec.gain_l[lane] = gain_l;
        vec.gain_r[lane] = gain_r;

        vec.unison[lane].voices = unison_voices;
        if (unison_voices > 1)
        {
            vec.unison[lane].phase = unison_phase;
            vec.unison[lane].ratio = unison_ratio;
            vec.unison[lane].mask = unison_mask;
            vec.unison[lane].gain_l = unison_gain_l;
            vec.unison[lane].gain_r = unison_gain_r;
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

        for (int lane = 0; lane < 4; ++lane) // for each voice
        {
            o.unison[lane].voices = vec.unison[lane].voices;
            if (o.unison[lane].voices == 1)
                continue;

            for (int batch = 0; batch < 4; batch++) // for each unison voice
            {
                auto idx = batch * 4;
                o.unison[lane].phase[batch].load(&vec.unison[lane].phase[idx]);
                o.unison[lane].inc[batch].load(&vec.unison[lane].ratio[idx]);
                o.unison[lane].inc[batch] *= vec.phase_inc[lane]; // convert ratio to increment
                o.unison[lane].mask[batch].load(&vec.unison[lane].mask[idx]);
            }
        }

        return o;
    }

    static OSCVec SIMDToVec(SIMDOSC simd)
    {
        OSCVec vec{};
        simd.phase.store(vec.phase);
        simd.level.store(vec.level);
        simd.out.store(vec.out);

        for (int lane = 0; lane < 4; ++lane)
        {
            vec.unison[lane].voices = simd.unison[lane].voices;
            if (simd.unison[lane].voices < 2)
                continue;

            for (int batch = 0; batch < 4; batch++)
            {
                int idx = batch * 4;
                simd.unison[lane].phase[batch].store(&vec.unison[lane].phase[idx]);
            }
        }

        return vec;
    }

    void vecToState(OSCVec& vec, int lane)
    {
        phase = vec.phase[lane];
        level = vec.level[lane];
        out = vec.out[lane];

        if (vec.unison[lane].voices > 1)
        {
            unison_phase = vec.unison[lane].phase;
        }
    }

private:
    TetraOPAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSC)
};
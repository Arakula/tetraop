#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "Utils.h"
#include "Unison.h"

using namespace globals;

class Synth;

class OSC
{
public:
    alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> unison_phases;

    float phase;
    float freq;
    float phase_inc;
    float level;
    float out;
    int uni_voices = 2;

    OSC() {}
    ~OSC() {}

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
        alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> gainL{};
        alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> gainR{};
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
        UnisonVec unison[4];
    };

    void stateToVec(OSCVec& vec, int lane, int oscIdx, Unison& uni) const
    {
        vec.phase[lane] = phase;
        vec.phase_inc[lane] = phase_inc;
        vec.freq[lane] = freq;
        vec.level[lane] = level;
        vec.out[lane] = out;

        int voices = uni.osc[oscIdx].voices;
        vec.unison[lane].voices = voices;

        if (voices > 1)
        {
            vec.unison[lane].phase = unison_phases;
            vec.unison[lane].ratio = uni.osc[oscIdx].ratio; // increments stored as ratios at this stage
            vec.unison[lane].mask = uni.osc[oscIdx].mask;
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
        simd.phase_inc.store(vec.phase_inc);
        simd.freq.store(vec.freq);
        simd.level.store(vec.level);
        simd.out.store(vec.out);

        for (int lane = 0; lane < 4; ++lane)
        {
            vec.unison[lane].voices = simd.unison[lane].voices;
            if (simd.unison[lane].voices < 2)
                continue;

            for (int batch = 0; batch < 4; batch++)
            {
                simd.unison[lane].phase[batch].store(&vec.unison[lane].phase[batch * 4]);
            }
        }

        return vec;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSC)
};
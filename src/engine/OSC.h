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
        SIMDF gain_l[4];
        SIMDF gain_r[4];
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
        SIMDF phase_offset;
        SIMDF pitch_ratio;
        SIMDF pitch_ratio_step;
        SIMDF gain_l;
        SIMDF gain_r;
        SIMDF freq;
        SIMDF level;
        SIMDF level_step;
        SIMDF out;
        SIMDF feedback;
        SIMDF morph;
        SIMDF morph_targ;
        SIMDUnison unison[4]; // four lanes of voices
    };

    struct OSCVec
    {
        alignas(sizeof(SIMDF)) float phase[4];
        alignas(sizeof(SIMDF)) float phase_inc[4];
        alignas(sizeof(SIMDF)) float phase_offset[4];
        alignas(sizeof(SIMDF)) float pitch_ratio[4];
        alignas(sizeof(SIMDF)) float pitch_ratio_step[4];
        alignas(sizeof(SIMDF)) float freq[4];
        alignas(sizeof(SIMDF)) float level[4];
        alignas(sizeof(SIMDF)) float level_step[4];
        alignas(sizeof(SIMDF)) float out[4];
        alignas(sizeof(SIMDF)) float feedback[4];
        alignas(sizeof(SIMDF)) float gain_l[4];
        alignas(sizeof(SIMDF)) float gain_r[4];
        alignas(sizeof(SIMDF)) float morph[4];
        alignas(sizeof(SIMDF)) float morph_targ[4];
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
    float phase_inc = 0.f;
    float phase_offset = 0.f;
    float freq = 1000.f;
    float level = 0.f;
    float level_targ = 0.f;
    float out = 0.f;
    float pan = -2.f;
    float gain_l = 0.f;
    float gain_r = 0.f;
    float feedback = 0.f;
    float morph = 0.f;
    float morph_targ = 0.f;
    int unison_voices = 1;
    int unison_mode = 0;
    float unison_detune = -1.f;
    float unison_stereo = -1.f;
    float unison_spread = -1.f;
    float unison_blend = -1.f;
    float pitch_ratio = 1.f;
    float pitch_ratio_targ = 1.f;

    OSC(int _id, int _voiceId, TetraOPAudioProcessor& p);
    ~OSC() {}

    OSC(OSC&&) = default;
    OSC& operator=(OSC&&) = default;

    void trigger(int note, float srate);
    void prepareBlock(int startSample, int numSamples);
    void finishBlock(int numSamples);
    void recalcUnison();

    void stateToVec(OSCVec& vec, int lane, bool isFMOutput, int numSamples) const
    {
        vec.phase[lane] = phase;
        vec.phase_inc[lane] = phase_inc;
        vec.phase_offset[lane] = phase_offset;
        vec.pitch_ratio[lane] = pitch_ratio;
        vec.pitch_ratio_step[lane] = (pitch_ratio_targ - pitch_ratio) / numSamples;
        vec.freq[lane] = freq;
        vec.level[lane] = level;
        vec.level_step[lane] = (level_targ - level) / numSamples;
        vec.out[lane] = out;
        vec.gain_l[lane] = gain_l;
        vec.gain_r[lane] = gain_r;
        vec.feedback[lane] = feedback;
        vec.morph[lane] = morph;
        vec.morph_targ[lane] = morph_targ;

        vec.unison[lane].voices = unison_voices;
        if (! isFMOutput || (level <= 1e-5f && level_targ < 1e-5f))
        {
            vec.unison[lane].voices = 1; // optimization, disable unison if its not processed
        }

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
        o.phase_offset.load(vec.phase_offset);
        o.pitch_ratio.load(vec.pitch_ratio);
        o.pitch_ratio_step.load(vec.pitch_ratio_step);
        o.freq.load(vec.freq);
        o.level.load(vec.level);
        o.level_step.load(vec.level_step);
        o.out.load(vec.out);
        o.gain_l.load(vec.gain_l);
        o.gain_r.load(vec.gain_r);
        o.feedback.load(vec.feedback);
        o.morph.load(vec.morph);
        o.morph_targ.load(vec.morph_targ);

        for (int lane = 0; lane < 4; ++lane) // for each voice
        {
            o.unison[lane].voices = vec.unison[lane].voices;
            if (o.unison[lane].voices == 1)
                continue;

            for (int batch = 0; batch < 4; batch++) // for each unison voice
            {
                auto idx = batch * 4;
                auto& uni = vec.unison[lane];
                o.unison[lane].phase[batch].load(&uni.phase[idx]);
                o.unison[lane].inc[batch].load(&uni.ratio[idx]);
                o.unison[lane].inc[batch] *= vec.phase_inc[lane]; // convert ratio to increment
                o.unison[lane].mask[batch].load(&uni.mask[idx]);
                o.unison[lane].gain_l[batch].load(&uni.gain_l[idx]);
                o.unison[lane].gain_r[batch].load(&uni.gain_r[idx]);
            }
        }

        return o;
    }

    static OSCVec SIMDToVec(SIMDOSC& simd)
    {
        OSCVec vec{};
        simd.phase.store(vec.phase);
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
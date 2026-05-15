#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "Utils.h"
#include "Unison.h"
#include "Modulation.h"

using namespace globals;

class TetraOPAudioProcessor;

class OSC
{
public:
    struct SIMDUnison
    {
        int voices = 1;
        SIMDF phase[4]; // four batches for each lane == MAX 16 unison voices per osc per voice
        SIMDF mask[4];
        SIMDF gain_l[4];
        SIMDF gain_r[4];
        SIMDF ratio[4];
    };

    struct SIMDOSC
    {
        bool isOn = false;
        bool morph_snap = false;
        SIMDF phase;
        SIMDF phase_inc;
        SIMDF phase_offset;
        SIMDF phase_offset_step;
        SIMDF pitch_ratio;
        SIMDF pitch_ratio_targ;
        SIMDF pitch_ratio_step;
        SIMDF gain_l;
        SIMDF gain_l_step;
        SIMDF gain_r;
        SIMDF gain_r_step;
        SIMDF freq;
        SIMDF level;
        SIMDF level_targ;
        SIMDF level_step;
        SIMDF out;
        SIMDF feedback;
        SIMDF morph;
        SIMDF morph_targ;
        SIMDF dist_amt;
        SIMDF dist_amt_targ;
        SIMDF dist_amt_step;
        SIMDUnison unison[4]; // four lanes of voices
        WhiteNoiseGen whiteNoiseGen[4];
        PinkNoiseGen pinkNoiseGen[4];
    };

    float srate = 44100.f;
    String prefix = "";
    int batch = 0; // SIMD group
    int lane = 0; // SIMD lane inside batch
    bool triggered = false; // process trigger logic inside startBlock

    int voiceId = 0;
    int id = 0;
    float level_targ = 0.f;
    float pitch_ratio_targ = 1.f;
    float pan = -2.f;
    int unison_mode = 0;
    float unison_detune = -1.f;
    float unison_stereo = -1.f;
    float unison_spread = -1.f;
    float unison_blend = -1.f;

    

    OSC(int _id, int _voiceId, TetraOPAudioProcessor& p);
    ~OSC() {}

    OSC(OSC&&) = default;
    OSC& operator=(OSC&&) = default;

    void trigger(int note, float srate);
    void retrigger(int note, float srate);
    void startBlock(int startSample, int numSamples);
    void recalcUnison(SIMDUnison& unison, int mod) const;

private:
    TetraOPAudioProcessor& audioProcessor;

    // params
    std::atomic<float>* morphSnapParam = nullptr;
    std::atomic<float>* onParam = nullptr;
    std::atomic<float>* unisonVoicesParam = nullptr;
    std::atomic<float>* unisonModeParam = nullptr;

    Modulation::Param* phaseOffsetParam = nullptr;
    Modulation::Param* phaseRandParam = nullptr;
    Modulation::Param* morphParam = nullptr;
    Modulation::Param* levelParam = nullptr;
    Modulation::Param* pitchCentsParam = nullptr;
    Modulation::Param* pitchSemisParam = nullptr;
    Modulation::Param* pitchOctParam = nullptr;
    Modulation::Param* panParam = nullptr;
    Modulation::Param* feedbackParam = nullptr;
    Modulation::Param* phaseDistAmtParam = nullptr;
    Modulation::Param* unisonDetuneParam = nullptr;
    Modulation::Param* unisonStereoParam = nullptr;
    Modulation::Param* unisonSpreadParam = nullptr;
    Modulation::Param* unisonBlendParam = nullptr;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSC)
};
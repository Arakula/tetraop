// Copyright 2026 tilr

#pragma once

#include <JuceHeader.h>
#include "Utils.h"
#include "OSC.h"
#include "Modulation.h"
#include "libMTSClient.h"

class TetraOPAudioProcessor;

//==============================================================================
class Voice : public gin::SynthesiserVoice
{
public:
    struct SIMDVoice
    {
        std::array<float, 4> key; // float 0..127 note pressed used to get wavetable
        float env_coeff = 1.f;
        SIMDF env;
        SIMDF env_targ;
        SIMDF vel_mult;
        SIMDF vel_step;

        // matrix coefficients
        // these are updated per block so that the FM matrix is fully modulatable (when layout is custom)
        // feedback is not present as it is updated per oscillator instead
        SIMDF fm_ab;
        SIMDF fm_ac;
        SIMDF fm_ad;
        SIMDF fm_ba;
        SIMDF fm_bc;
        SIMDF fm_bd;
        SIMDF fm_ca;
        SIMDF fm_cb;
        SIMDF fm_cd;
        SIMDF fm_da;
        SIMDF fm_db;
        SIMDF fm_dc;

        SIMDF fm_aout;
        SIMDF fm_bout;
        SIMDF fm_cout;
        SIMDF fm_dout;

        SIMDF rm_aa;
        SIMDF rm_ab;
        SIMDF rm_ac;
        SIMDF rm_ad;
        SIMDF rm_ba;
        SIMDF rm_bb;
        SIMDF rm_bc;
        SIMDF rm_bd;
        SIMDF rm_ca;
        SIMDF rm_cb;
        SIMDF rm_cc;
        SIMDF rm_cd;
        SIMDF rm_da;
        SIMDF rm_db;
        SIMDF rm_dc;
        SIMDF rm_dd;
    };

	int id;
    int batch; // SIMD group
    int lane; // SIMD lane inside batch
    uint64_t pressed_ts = 1; // timestamp used for rand generators based on note start
    float srate = 44100.f;
    float israte = 0.0001f;
    float attack_elapsed = 0.0f; // elapsed time in seconds
    float release_elapsed = 0.0f;
    bool released = false;
    bool pressed = false;
    bool fading = false;
    float vel = 0.f; // norm velocity
    float key = 0.f; // norm note used for keytracking
    int mpe_channel = 1;

    float fastKillGain = 1.f; // fastkill fadeout gain
    float fastKillStep = 1.f; // fastkill gain increment per sample

    float freq = 0.f;
    float glide_elapsed = 0;
    bool glide = false;
    float glide_start = 0;
    float glide_targ = 0;
    float glide_curr = 0;

    std::vector<OSC> osc;

    Voice (TetraOPAudioProcessor& p, int id);

    void clear();

    float getCurrentNote() override { return key * 127.0f; }
    void updateFreq(float keyval);

    void noteStarted() override;
    void noteRetriggered() override;
    void noteStopped (bool allowTailOff) override;

    void notePressureChanged() override;
    void noteTimbreChanged() override;
    void notePitchbendChanged() override    {}
    void noteKeyStateChanged() override     {}

    void setCurrentSampleRate (double newRate) override;
    void renderNextBlock(AudioBuffer<float>&, int, int) override {}
    void startBlock(int startSample, int numSamples);
    void endBlock(int startSample, int numSamples);

    void updateFilters(bool init, int blkoffset = 0);
    float getKeytrackCutoff(float cutoff, float keytrack);
    void updateMatrix(SIMDVoice& voice, int blkoffset = 0);

private:
    static inline uint64_t pressed_ts_counter = 1;
    TetraOPAudioProcessor& audioProcessor;

    float ampKeyTrack = 1.0f;
    double phase = 0.f;

    // interpolation
    float vel_targ = 0.f;
    float vel_step = 0.f;

    // params
    std::atomic<float>* glideParam = nullptr;
    std::atomic<float>* glideTensionParam = nullptr;
    std::atomic<float>* legatoParam = nullptr;
    std::atomic<float>* velSenseParam = nullptr;
    std::atomic<float>* f1OnParam = nullptr;
    std::atomic<float>* f1KTrackParam = nullptr;
    std::atomic<float>* f2OnParam = nullptr;
    std::atomic<float>* f2KTrackParam = nullptr;

    Modulation::Param* f1CutParam = nullptr;
    Modulation::Param* f1ResParam = nullptr;
    Modulation::Param* f1DriveParam = nullptr;
    Modulation::Param* f1MixParam = nullptr;
    Modulation::Param* f2CutParam = nullptr;
    Modulation::Param* f2ResParam = nullptr;
    Modulation::Param* f2DriveParam = nullptr;
    Modulation::Param* f2MixParam = nullptr;
};

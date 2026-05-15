#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "FmMatrix.h"
#include "Voice.h"
#include "Utils.h"
#include "PhaseDist.h"
#include "TablesManager.h"

using namespace globals;
using RenderFn = SIMDF(*)(const std::array<float*, 8>&, int, SIMDF, SIMDF, OSC::SIMDOSC&);

class TetraOPAudioProcessor;

struct SIMDVox
{
    Voice::SIMDVoice voice;
    OSC::SIMDOSC osc[4];
};

class FmMatrix : public juce::AudioProcessorValueTreeState::Listener
{
public:
    struct TablesData
    {
        SIMDF currIndex;
        SIMDF targIndex;
        int numTables;
        int size;
        bool isPinkNoise;
        bool isWhiteNoise;
        bool isMorphing;
        std::array<float*, 8> data; // 4 tables (1 per voice) + 4 morph target tables
    };

    using Matrix4x4 = std::array<std::array<SIMDF, 4>, 4>;

    enum Layout {
        A_B_C_D,
        DCBA,
        CA_DB,
        DB_C_A,
        DA_DB_DC,
        BA_CA_DA,
        A_CB_DC,
        DC_CA_CB,
        DC_DB_BA_CA,
        DB_CB_BA,
        Custom,
        kLayouts,
    };
    bool layoutDirty = true;

    Layout layout = Layout::A_B_C_D;
    std::array<Matrix4x4, kLayouts> layouts{};
    Matrix4x4 matrix{}; // fm matrix
    Matrix4x4 rmmtx; // rm matrix
    bool hasRM = false;

    SIMDF ab = 0.f; SIMDF ac = 0.f; SIMDF ad = 0.f;
    SIMDF ba = 0.f; SIMDF bc = 0.f; SIMDF bd = 0.f;
    SIMDF ca = 0.f; SIMDF cb = 0.f; SIMDF cd = 0.f;
    SIMDF da = 0.f; SIMDF db = 0.f; SIMDF dc = 0.f;

    // Output buffers for each oscillator
    std::array<SIMDF, MAX_BLOCKSIZE> outL[4];
    std::array<SIMDF, MAX_BLOCKSIZE> outR[4];

    // oscilloscope sampling
    std::array<float, SCOPE_BUFLEN> oscOut[4]{};
    std::array<int, 4> lastScopeIdx{};
    uint32_t sampleCounter = 0;

    SIMDF AisOut = 0.f;
    SIMDF BisOut = 0.f;
    SIMDF CisOut = 0.f;
    SIMDF DisOut = 0.f;
    SIMDF isOut[4] = {};

    // phase distortion function pointers
    DistFn Adist = PhaseDist::bypass;
    DistFn Bdist = PhaseDist::bypass;
    DistFn Cdist = PhaseDist::bypass;
    DistFn Ddist = PhaseDist::bypass;
    WindowFn Awindow = PhaseDist::windowBypass;
    WindowFn Bwindow = PhaseDist::windowBypass;
    WindowFn Cwindow = PhaseDist::windowBypass;
    WindowFn Dwindow = PhaseDist::windowBypass;

    FmMatrix(TetraOPAudioProcessor& p);
    ~FmMatrix();

    void parameterChanged(const juce::String& paramId, float value) override;

    void refreshLayout(SIMDVox& vox);
    void prepareDistortions(SIMDVox& vox);
    void setLayout(Layout l, SIMDVox& vox);
    void prepare(float _srate);
    void processBlock(SIMDVox& data, int numSamples, int activeVoiceLane, SIMDF vmask);
    TablesData getTables(SIMDVox& vox, int oscidx, bool isMorphing, SIMDF isOut);

    inline static SIMDF renderSine(SIMDF phase)
    {
        Utils::wrapPhase(phase);
        return mipp::sin(phase * MathConstants<float>::twoPi);
    }

    static inline SIMDF renderWhiteNoise(const std::array<float*, 8>&, const int, SIMDF, const SIMDF, OSC::SIMDOSC& osc)
    {
        return
        {
            osc.whiteNoiseGen[0].next(),
            osc.whiteNoiseGen[1].next(),
            osc.whiteNoiseGen[2].next(),
            osc.whiteNoiseGen[3].next(),
        };
    }

    static inline SIMDF renderPinkNoise(const std::array<float*, 8>&, const int, SIMDF, const SIMDF, OSC::SIMDOSC& osc)
    {
        return
        {
            osc.pinkNoiseGen[0].next(),
            osc.pinkNoiseGen[1].next(),
            osc.pinkNoiseGen[2].next(),
            osc.pinkNoiseGen[3].next(),
        };
    }

    // last arg void* is to keep the same signature as noise generators
    static inline SIMDF renderWaveLinear(const std::array<float*, 8>& tables, const int size, SIMDF phase, const SIMDF morph, OSC::SIMDOSC&)
    {
        Utils::wrapPhase(phase);
        auto posf = phase * float(size);
        auto posi = posf.trunc();
        auto frac = posf - posi;
        auto posint = mipp::cvt<float, int32_t>(posi) + 1; // tables are padded for cubic interpolation, add 1

        int p[4];
        posint.store(p);

        SIMDF l1 = SIMDF{ tables[0][p[0]],     tables[1][p[1]],     tables[2][p[2]],     tables[3][p[3]] };
        SIMDF l2 = SIMDF{ tables[0][p[0] + 1], tables[1][p[1] + 1], tables[2][p[2] + 1], tables[3][p[3] + 1] };

        if (morph.hmax() > 0.f)
        {
            SIMDF l3 = SIMDF{ tables[4][p[0]],     tables[5][p[1]],     tables[6][p[2]],     tables[7][p[3]] };
            SIMDF l4 = SIMDF{ tables[4][p[0] + 1], tables[5][p[1] + 1], tables[6][p[2] + 1], tables[7][p[3] + 1] };

            l1 = mipp::fmadd(morph, (l3 - l1), l1);
            l2 = mipp::fmadd(morph, (l4 - l2), l2);
        }

        return mipp::fmadd(frac, (l2 - l1), l1);
    }

    static inline SIMDF renderWaveCubic(const std::array<float*, 8>& tables, const int size, SIMDF phase, const SIMDF morph, OSC::SIMDOSC&)
    {
        Utils::wrapPhase(phase);
        auto posf = phase * float(size);
        auto posi = posf.trunc();
        auto t = posf - posi;
        auto t2 = t * t;
        auto t3 = t2 * t;
        auto posint = mipp::cvt<float, int32_t>(posi) + 1; // tables are padded for cubic interpolation, add 1

        int p[4];
        posint.store(p);

        SIMDF y0 = SIMDF{ tables[0][p[0] - 1], tables[1][p[1] - 1], tables[2][p[2] - 1], tables[3][p[3] - 1] };
        SIMDF y1 = SIMDF{ tables[0][p[0]],     tables[1][p[1]],     tables[2][p[2]],     tables[3][p[3]] };
        SIMDF y2 = SIMDF{ tables[0][p[0] + 1], tables[1][p[1] + 1], tables[2][p[2] + 1], tables[3][p[3] + 1] };
        SIMDF y3 = SIMDF{ tables[0][p[0] + 2], tables[1][p[1] + 2], tables[2][p[2] + 2], tables[3][p[3] + 2] };

        if (morph.hmax() > 0.f)
        {
            SIMDF y02 = SIMDF{ tables[4][p[0] - 1], tables[5][p[1] - 1], tables[6][p[2] - 1], tables[7][p[3] - 1] };
            SIMDF y12 = SIMDF{ tables[4][p[0]],     tables[5][p[1]],     tables[6][p[2]],     tables[7][p[3]] };
            SIMDF y22 = SIMDF{ tables[4][p[0] + 1], tables[5][p[1] + 1], tables[6][p[2] + 1], tables[7][p[3] + 1] };
            SIMDF y32 = SIMDF{ tables[4][p[0] + 2], tables[5][p[1] + 2], tables[6][p[2] + 2], tables[7][p[3] + 2] };

            y0 = mipp::fmadd(morph, (y02 - y0), y0);
            y1 = mipp::fmadd(morph, (y12 - y1), y1);
            y2 = mipp::fmadd(morph, (y22 - y2), y2);
            y3 = mipp::fmadd(morph, (y32 - y3), y3);
        }

        auto c1 = y0.fmadd(-0.5f, y1.fmadd(1.5f, (-y2).fmadd(1.5f, y3 * 0.5f)));
        auto c2 = y0 + (-y1).fmadd(2.5f, y2.fmadd(2.f, -y3 * 0.5f));
        auto c3 = y0.fmadd(-0.5f, y2 * 0.5f);
        return c1.fmadd(t3, c2.fmadd(t2, c3.fmadd(t, y1)));
    }

    inline void interpolateOSC(OSC::SIMDOSC& osc)
    {
        osc.level += osc.level_step;
        osc.phase = osc.phase_inc.fmadd(osc.pitch_ratio, osc.phase);
        osc.pitch_ratio += osc.pitch_ratio_step;
        osc.gain_l += osc.gain_l_step;
        osc.gain_r += osc.gain_r_step;
        osc.phase_offset = (osc.phase_offset + osc.phase_offset_step).sat(0.f, 1.f);
        osc.dist_amt = (osc.dist_amt + osc.dist_amt_step).sat(-1.0f, 1.0f);
        Utils::wrapPhase(osc.phase);
    }

    inline void sampleOscilloscope(OSC::SIMDOSC& osc, SIMDF& out_l, SIMDF& out_r, int oscIdx, int voice)
    {
        int idx = (int)(osc.phase.get(voice) * SCOPE_BUFLEN);
        int lastIdx = lastScopeIdx[oscIdx];
        if (idx == lastIdx) return;

        float val = ((out_l + out_r) * 0.5f).get(voice);
        int start = lastIdx;
        int end = idx;
        if (end < start) end += SCOPE_BUFLEN; // handle wrap-around

        int span = end - start;
        if (span == 1)
        {
            oscOut[oscIdx][idx] = val;
        }
        else
        {
            float lastVal = oscOut[oscIdx][lastIdx];
            float delta = (val - lastVal) / span;
            float cur = lastVal;
            int writeIdx = (start + 1) % SCOPE_BUFLEN;

            for (int i = 0; i < span; ++i)
            {
                cur += delta;
                oscOut[oscIdx][writeIdx] = cur;

                writeIdx++;
                if (writeIdx == SCOPE_BUFLEN)
                    writeIdx = 0;
            }
        }
        lastScopeIdx[oscIdx] = idx;
    }

private:
    template<bool ring, bool AOn, bool BOn, bool COn, bool DOn>
    void _process(SIMDVox& data, int numSamples, const int activeVoiceLane, SIMDF vmask);
    float morphAlpha = 0.f; // exponential param smoother

	TetraOPAudioProcessor& audioProcessor;
    float srate;

    const SIMDF zero = 0.f;
    const SIMDF one = 1.f;
    SIMDF aout;
    SIMDF bout;
    SIMDF cout;
    SIMDF dout;

    std::atomic<float>* a_phase_dist_mode_param;
    std::atomic<float>* b_phase_dist_mode_param;
    std::atomic<float>* c_phase_dist_mode_param;
    std::atomic<float>* d_phase_dist_mode_param;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FmMatrix)
};
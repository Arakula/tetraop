#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "FmMatrix.h"
#include "Voice.h"
#include "Utils.h"

using namespace globals;
class TetraOPAudioProcessor;

struct SIMDVox
{
    Voice::SIMDVoice voice;
    OSC::SIMDOSC osc[4];
};

class FmMatrix
{
public:
    struct TablesData 
    {
        SIMDF currIndex;
        SIMDF targIndex;
        int numTables;
        std::array<float*, 8> data;
    };

    using Matrix4x4 = std::array<std::array<float, 4>, 4>;

    enum Layout {
        A_B_C_D,
        DCBA,
        DC_BA,
        DC_B_A,
        DA_DB_DC,
        BA_CA_DA,
        A_CB_DC,
        DC_CA_CB,
        DC_DB_BA_CA,
        DB_CB_BA,
        kLayouts,
    };

    Layout layout = Layout::A_B_C_D;
    std::array<Matrix4x4, kLayouts> matrices{};
    Matrix4x4 matrix{};

    float ab = 0.f; float ac = 0.f; float ad = 0.f;
    float ba = 0.f; float bc = 0.f; float bd = 0.f;
    float ca = 0.f; float cb = 0.f; float cd = 0.f;
    float da = 0.f; float db = 0.f; float dc = 0.f;

    std::array<SIMDF, MAX_BLOCKSIZE> outL;
    std::array<SIMDF, MAX_BLOCKSIZE> outR;

    bool AisOut = false;
    bool BisOut = false;
    bool CisOut = false;
    bool DisOut = false;
    bool isOut[4] = {};

    FmMatrix(TetraOPAudioProcessor& p);
    ~FmMatrix();

    void setLayout(Layout l);
    void prepare(float _srate);
    void processBlock(SIMDVox& data, int numSamples);

private:
    TablesData getTables(SIMDVox& vox, int oscidx, bool isMorphing);

    template<bool AOn, bool BOn, bool COn, bool DOn>
    void _process(SIMDVox& data, int numSamples);
    float morphAlpha = 0.f; // exponential param smoother coeff

	TetraOPAudioProcessor& audioProcessor;
    float srate;

    const SIMDF zero = 0.f;
    const SIMDF one = 1.f;
    SIMDF aout;
    SIMDF bout;
    SIMDF cout;
    SIMDF dout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FmMatrix)
};
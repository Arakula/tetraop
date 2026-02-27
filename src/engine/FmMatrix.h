#pragma once

#include <JuceHeader.h>
#include "../Globals.h"
#include "FmMatrix.h"

using namespace globals;

class TetraOPAudioProcessor;

class FmMatrix
{
public:
    using Matrix4x4 = std::array<std::array<int, 4>, 4>;

    struct OSC {
        bool on = false;
        float phase = 0.f;
        float phaseInc = 0.f;
        float level = 0.5;
        float freq = 0.5;
        float out = 0.f;
    };

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

    FmMatrix(TetraOPAudioProcessor& p);
    ~FmMatrix();

    void setLayout(Layout l);
    void prepare(float _srate);
    void renderBlock (gin::ScratchBuffer& buf, int blockoffset, int numSamples);

private:
	TetraOPAudioProcessor& audioProcessor;
    float srate;

    bool AisOut = true;
    bool BisOut = true;
    bool CisOut = true;
    bool DisOut = true;

    OSC A;
    OSC B;
    OSC C;
    OSC D;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FmMatrix)
};
#include "FmMatrix.h"

FmMatrix::FmMatrix(TetraOPAudioProcessor& p) : audioProcessor(p)
{
    matrices[int(DCBA)][3][2] = 1.f; // DC
    matrices[int(DCBA)][2][1] = 1.f; // CB
    matrices[int(DCBA)][1][0] = 1.f; // BA

    matrices[int(DC_BA)][3][2] = 1.f; // DC
    matrices[int(DC_BA)][1][0] = 1.f; // BA

    matrices[int(DC_B_A)][3][2] = 1.f; // DC

    matrices[int(DA_DB_DC)][3][0] = 1.f; // DA
    matrices[int(DA_DB_DC)][3][1] = 1.f; // DB
    matrices[int(DA_DB_DC)][3][2] = 1.f; // DC

    matrices[int(BA_CA_DA)][1][0] = 1.f; // BA
    matrices[int(BA_CA_DA)][2][0] = 1.f; // CA
    matrices[int(BA_CA_DA)][3][0] = 1.f; // DA

    matrices[int(A_CB_DC)][2][1] = 1.f; // CB
    matrices[int(A_CB_DC)][3][2] = 1.f; // DC

    matrices[int(DC_CA_CB)][3][2] = 1.f; // DC
    matrices[int(DC_CA_CB)][2][0] = 1.f; // CA
    matrices[int(DC_CA_CB)][2][1] = 1.f; // CB

    matrices[int(DC_DB_BA_CA)][3][2] = 1.f; // DC
    matrices[int(DC_DB_BA_CA)][3][1] = 1.f; // DB
    matrices[int(DC_DB_BA_CA)][1][0] = 1.f; // BA
    matrices[int(DC_DB_BA_CA)][2][0] = 1.f; // CA

    matrices[int(DB_CB_BA)][3][1] = 1.f; // DB
    matrices[int(DB_CB_BA)][2][1] = 1.f; // CB
    matrices[int(DB_CB_BA)][1][0] = 1.f; // BA

    setLayout(Layout::DCBA);
}

FmMatrix::~FmMatrix()
{
}

void FmMatrix::setLayout(Layout l)
{
    layout = l;
    matrix = matrices[layout];

    auto isoutput = [this](int row) {
        return !matrix[row][0] && !matrix[row][1] && !matrix[row][2] && !matrix[row][3];
    };

    AisOut = isoutput(0); isOut[0] = AisOut;
    BisOut = isoutput(1); isOut[1] = BisOut;
    CisOut = isoutput(2); isOut[2] = CisOut;
    DisOut = isoutput(3); isOut[3] = DisOut;
}

void FmMatrix::prepare(float _srate)
{
    srate = _srate;
}

/*
static float render(float phase)
{
    if (phase >= 0.f) {
        phase -= std::trunc(phase);
    } else {
        phase += 1 - (std::trunc(phase));
    }
    return std::sin(phase * MathConstants<float>::twoPi);
}

void FmMatrix::renderBlock (gin::ScratchBuffer& buffer, int blockoffset, int numSamples)
{
    auto l = buffer.getWritePointer(0);
    auto r = buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        A.out = render(A.phase + B.out * matrix[1][0]) * A.level;
        A.phase += A.phaseInc;
        if (A.phase > 1) A.phase -= 1;

        B.out = render(B.phase + A.out * matrix[0][1]) * B.level;
        B.phase += B.phaseInc;
        if (B.phase > 1) B.phase -= 1;

        *l++ += A.out;
        *r++ += A.out;
    }
}
*/

SIMDF FmMatrix::renderSIMD(SIMDF phase)
{
    SIMDF phaseTrunc = mipp::trunc(phase);
    auto mask = phase >= zero;
    
    phase = mipp::blend(phase - phaseTrunc, phase + one - phaseTrunc, mask);
    return mipp::sin(phase * MathConstants<float>::twoPi);
}

// SIMD'ed voices rendering
void FmMatrix::processBlock(SIMDVox& data, int numSamples)
{
    auto& A = data.osc[0];
    auto& B = data.osc[1];
    SIMDM mask;
    SIMDF out;

    for (int i = 0; i < numSamples; ++i)
    {
        A.out = renderSIMD(A.phase + B.out * matrix[1][0]) * A.level;
        B.out = renderSIMD(B.phase + A.out * matrix[0][1]) * B.level;

        // increment phases
        for (int j = 0; j < MAX_OPERATORS; ++j) 
        {
            auto& osc = data.osc[j];
            osc.phase += osc.phase_inc;
            mask = osc.phase > one;
            osc.phase = mipp::blend(osc.phase - one, osc.phase, mask);
        }

        // render output
        out = A.out * AisOut + B.out * BisOut;
        outL[i] = out;
        outR[i] = out;
    }
}
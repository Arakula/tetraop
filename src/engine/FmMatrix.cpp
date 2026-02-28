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
    Utils::wrapPhase(phase);
    return mipp::sin(phase * MathConstants<float>::twoPi);
}

std::pair<SIMDF, SIMDF> FmMatrix::processUnison(OSC::SIMDOSC& osc, SIMDF phaseOffset)
{
    alignas(sizeof(SIMDF)) float accL[4] = { 0.f, 0.f, 0.f, 0.f };
    alignas(sizeof(SIMDF)) float accR[4] = { 0.f, 0.f, 0.f, 0.f };

    for (int lane = 0; lane < 4; ++lane) // for each voice
    {
        auto& U = osc.unison[lane];
        if (U.voices == 1) continue; // unused voice

        int batch = (U.voices + 3) >> 2;
        auto offset = SIMDF(phaseOffset.get(lane));

        for (int v = 0; v < batch; ++v) // for each unison voice
        {
            SIMDF s = renderSIMD(U.phase[v] + offset);
            s *= U.mask[v];

            accL[lane] += (s * 1.f).sum();
            accR[lane] += (s * 1.f).sum();

            U.phase[v] += U.inc[v];
            Utils::wrapPhase(U.phase[v]);
        }
    }

    return { mipp::load(accL), mipp::load(accR) };
}

// SIMD'ed voices rendering
void FmMatrix::processBlock(SIMDVox& data, int numSamples)
{
    auto& A = data.osc[0];
    auto& B = data.osc[1];
    SIMDF la, lb, lc, ld;
    SIMDF offsetA, offsetB;
    SIMDF AoutL;
    SIMDF AoutR;
    SIMDF BoutL;
    SIMDF BoutR;

    for (int i = 0; i < numSamples; ++i)
    {
        la = A.out; lb = B.out;

        // compute mono outputs
        offsetA = lb * matrix[1][0];
        offsetB = la * matrix[0][1];
        A.out = renderSIMD(A.phase + offsetA) * A.level;
        B.out = renderSIMD(B.phase + offsetB) * B.level;

        AoutL = AoutR = A.out * AisOut;
        BoutL = BoutR = B.out * BisOut;

        // compute unison
        if (AisOut && A.unison->voices > 1)
        {
            auto [uniL, uniR] = processUnison(A, offsetA);
            AoutL = uniL * A.level;
            AoutR = uniR * A.level;
        }

        if (BisOut && B.unison->voices > 1)
        {
            auto [uniL, uniR] = processUnison(B, offsetB);
            BoutL = uniL * B.level;
            BoutR = uniR * B.level;
        }

        // increment phases
        for (int j = 0; j < MAX_OPERATORS; ++j) 
        {
            auto& osc = data.osc[j];
            osc.phase += osc.phase_inc;
            Utils::wrapPhase(osc.phase);
        }

        // render output
        outL[i] = AoutL + BoutL;
        outR[i] = AoutR + BoutR;
    }
}
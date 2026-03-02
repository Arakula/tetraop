#include "FmMatrix.h"
#include "../PluginProcessor.h"

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

    ab = matrix[0][1]; ac = matrix[0][2]; ad = matrix[0][3];
    ba = matrix[1][0]; bc = matrix[1][2]; bd = matrix[1][3];
    ca = matrix[2][0]; cb = matrix[2][1]; cd = matrix[2][3];
    da = matrix[3][0]; db = matrix[3][1]; dc = matrix[3][2];

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

SIMDF FmMatrix::renderSIMD(SIMDF phase)
{
    Utils::wrapPhase(phase);
    return mipp::sin(phase * MathConstants<float>::twoPi);
}

static inline SIMDF renderUnison(const std::vector<float> table, int size, SIMDF phase)
{
    static constexpr float almostOne = 1.f - std::numeric_limits<float>::epsilon();

    Utils::wrapPhase(phase);
    auto pos = phase.min(almostOne) * (float)size;
    auto frac = pos - (pos = pos.trunc());
    auto posi = mipp::cvt<float, int32_t>(pos);

    SIMDF l1 = mipp::gather(&table[0], posi);
    SIMDF l2 = mipp::gather(&table[0], posi + 1);

    return l1 + frac * (l2 - l1);
}

static inline SIMDF renderWave(const std::vector<float> table, int size, SIMDF phase)
{
    static constexpr float almostOne = 1.f - std::numeric_limits<float>::epsilon();

    Utils::wrapPhase(phase);
    auto pos = phase.min(almostOne) * (float)size;
    auto frac = pos - (pos = pos.trunc());
    auto posi = mipp::cvt<float, int32_t>(pos);

    int p[4];
    posi.store(p);
    float f[4];
    frac.store(f);

    SIMDF l1 = SIMDF{ table[p[0]], table[p[1]], table[p[2]], table[p[3]] };
    SIMDF l2 = SIMDF{ table[p[0] + 1], table[p[1] + 1], table[p[2] + 1], table[p[3] + 1] };

    return l1 + frac * (l2 - l1);
}

std::pair<SIMDF, SIMDF> FmMatrix::processUnison(OSC::SIMDOSC& osc, SIMDF phaseOffset)
{
    alignas(sizeof(SIMDF)) float accL[4] = { 0.f, 0.f, 0.f, 0.f };
    alignas(sizeof(SIMDF)) float accR[4] = { 0.f, 0.f, 0.f, 0.f };

    for (int lane = 0; lane < 4; ++lane) // for each voice
    {
        auto& U = osc.unison[lane];
        int batch = (U.voices + 3) >> 2;
        auto offset = SIMDF(phaseOffset.get(lane));

        for (int v = 0; v < batch; ++v) // for each unison voice
        {
            SIMDF s = renderSIMD(U.phase[v] + offset);
            s *= U.mask[v];

            accL[lane] += (s * U.gain_l[v]).sum();
            accR[lane] += (s * U.gain_r[v]).sum();

            U.phase[v] += U.inc[v];
            Utils::wrapPhase(U.phase[v]);
        }
    }

    return { mipp::load(accL), mipp::load(accR) };
}



// SIMD'ed voices rendering
void FmMatrix::processBlock(SIMDVox& data, int numSamples)
{
    auto& tables = audioProcessor.wavetables[0].tables;
    auto tableIndex = std::min(tables.size() - 1, int(float(tables.size()) * 0.5f));
    auto& table = tables.getTable(tableIndex)->tableForNote(0.5f);
    int size = tables.getTable(tableIndex)->tableSize;

    auto& A = data.osc[0];
    auto& B = data.osc[1];
    auto& C = data.osc[2];
    auto& D = data.osc[3];

    bool AhasUnison = AisOut && A.unison->voices > 1;
    bool BhasUnison = BisOut && B.unison->voices > 1;
    bool ChasUnison = CisOut && C.unison->voices > 1;
    bool DhasUnison = DisOut && D.unison->voices > 1;

    SIMDF la, lb, lc, ld;
    SIMDF offsetA, offsetB, offsetC, offsetD;
    SIMDF AoutL, AoutR;
    SIMDF BoutL, BoutR;
    SIMDF CoutL, CoutR;
    SIMDF DoutL, DoutR;

    for (int i = 0; i < numSamples; ++i)
    {
        la = A.out; lb = B.out;
        lc = C.out; ld = D.out;

        // compute mono outputs
        offsetA = la * A.feedback + lb * ba           + lc * ca           + ld * da;
        offsetB = la * ab         + lb * B.feedback   + lc * cb           + ld * db;
        offsetC = la * ac         + lb * bc           + lc * C.feedback   + ld * dc;
        offsetD = la * ad         + lb * bd           + lc * cd           + ld * D.feedback;

        A.out = renderWave(table, size, A.phase + offsetA) * A.level;
        B.out = renderWave(table, size, B.phase + offsetB) * B.level;
        C.out = renderWave(table, size, C.phase + offsetC) * C.level;
        D.out = renderWave(table, size, D.phase + offsetD) * D.level;

        AoutL = AoutR = A.out * AisOut;
        BoutL = BoutR = B.out * BisOut;
        CoutL = CoutR = C.out * CisOut;
        DoutL = DoutR = D.out * DisOut;

        // compute unison
        if (AhasUnison)
        {
            auto [uniL, uniR] = processUnison(A, offsetA);
            AoutL = uniL * A.level;
            AoutR = uniR * A.level;
        }

        if (BhasUnison)
        {
            auto [uniL, uniR] = processUnison(B, offsetB);
            BoutL = uniL * B.level;
            BoutR = uniR * B.level;
        }

        if (ChasUnison)
        {
            auto [uniL, uniR] = processUnison(C, offsetC);
            CoutL = uniL * C.level;
            CoutR = uniR * C.level;
        }

        if (DhasUnison)
        {
            auto [uniL, uniR] = processUnison(D, offsetD);
            DoutL = uniL * D.level;
            DoutR = uniR * D.level;
        }

        // increment phases
        for (int j = 0; j < MAX_OSCILLATORS; ++j)
        {
            auto& osc = data.osc[j];
            osc.phase += osc.phase_inc;
            Utils::wrapPhase(osc.phase);
        }

        // render output
        outL[i] = AoutL * A.gain_l + BoutL * B.gain_l + CoutL * C.gain_l + DoutL * D.gain_l;
        outR[i] = AoutR * A.gain_r + BoutR * B.gain_r + CoutR * C.gain_r + DoutR * D.gain_r;
    }
}
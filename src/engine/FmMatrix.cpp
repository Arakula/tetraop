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
    alpha = 1.f - std::exp(-1.f / (0.002f * srate));
}

inline static SIMDF renderSIMD(SIMDF phase)
{
    Utils::wrapPhase(phase);
    return mipp::sin(phase * MathConstants<float>::twoPi);
}

static inline SIMDF renderUnison(const std::vector<float> table, int size, SIMDF phase)
{
    static constexpr float almostOne = 1.f - std::numeric_limits<float>::epsilon();

    Utils::wrapPhase(phase);
    auto posf = phase * float(size);
    auto posi = posf.trunc();
    auto frac = posf - posi;
    auto posint = mipp::cvt<float, int32_t>(posi);

    SIMDF l1 = mipp::gather(&table[0], posint);
    SIMDF l2 = mipp::gather(&table[0], posint + 1);

    return l1 + frac * (l2 - l1);
}

static inline SIMDF renderWave(const float* table, int size, SIMDF phase)
{
    Utils::wrapPhase(phase);
    auto posf = phase * float(size);
    auto posi = posf.trunc();
    auto frac = posf - posi;
    auto posint = mipp::cvt<float, int32_t>(posi);

    int p[4];
    posint.store(p);
    float f[4];
    frac.store(f);

    SIMDF l1 = SIMDF{ table[p[0]], table[p[1]], table[p[2]], table[p[3]] };
    SIMDF l2 = SIMDF{ table[p[0] + 1], table[p[1] + 1], table[p[2] + 1], table[p[3] + 1] };

    return l1 + frac * (l2 - l1);
}

static inline std::pair<SIMDF, SIMDF> processUnison(OSC::SIMDOSC& osc, SIMDF phaseOffset)
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
    bool aon = data.osc[0].level.sum() > 0.f;
    bool bon = data.osc[1].level.sum() > 0.f;
    bool con = data.osc[2].level.sum() > 0.f;
    bool don = data.osc[3].level.sum() > 0.f;

    int mask = (aon << 3) | (bon << 2) | (con << 1) | (don << 0);

    switch (mask)
    {
    case 0b0000: _process<false, false, false, false>(data, numSamples); break;
    case 0b0001: _process<false, false, false, true>(data, numSamples); break;
    case 0b0010: _process<false, false, true, false>(data, numSamples); break;
    case 0b0011: _process<false, false, true, true>(data, numSamples); break;
    case 0b0100: _process<false, true, false, false>(data, numSamples); break;
    case 0b0101: _process<false, true, false, true>(data, numSamples); break;
    case 0b0110: _process<false, true, true, false>(data, numSamples); break;
    case 0b0111: _process<false, true, true, true>(data, numSamples); break;
    case 0b1000: _process<true, false, false, false>(data, numSamples); break;
    case 0b1001: _process<true, false, false, true>(data, numSamples); break;
    case 0b1010: _process<true, false, true, false>(data, numSamples); break;
    case 0b1011: _process<true, false, true, true>(data, numSamples); break;
    case 0b1100: _process<true, true, false, false>(data, numSamples); break;
    case 0b1101: _process<true, true, false, true>(data, numSamples); break;
    case 0b1110: _process<true, true, true, false>(data, numSamples); break;
    case 0b1111: _process<true, true, true, true>(data, numSamples); break;
    }
}

template<bool AOn, bool BOn, bool COn, bool DOn>
void FmMatrix::_process(SIMDVox& data, int numSamples)
{
    auto& A = data.osc[0];
    auto& B = data.osc[1];
    auto& C = data.osc[2];
    auto& D = data.osc[3];

    bool AisMorphing = AOn && std::abs((A.morph - A.morph_targ).sum()) > 1e-4f;
    bool BisMorphing = BOn && std::abs((B.morph - B.morph_targ).sum()) > 1e-4f;
    bool CisMorphing = COn && std::abs((C.morph - C.morph_targ).sum()) > 1e-4f;
    bool DisMorphing = DOn && std::abs((D.morph - D.morph_targ).sum()) > 1e-4f;

    auto& tables = audioProcessor.wavetables[0].tables;
    auto tableIndex = std::min(tables.size() - 1, int(float(tables.size()) * 0.0f));
    auto& table = tables.getUnchecked(tableIndex)->tableForNote(data.voice.key[0]);
    auto ttt = tables.getUnchecked(tableIndex);
    int size = tables.getUnchecked(tableIndex)->tableSize;

    bool AhasUnison = AisOut && A.unison->voices > 1;
    bool BhasUnison = BisOut && B.unison->voices > 1;
    bool ChasUnison = CisOut && C.unison->voices > 1;
    bool DhasUnison = DisOut && D.unison->voices > 1;

    SIMDF la, lb, lc, ld;
    SIMDF offsetA{}, offsetB{}, offsetC{}, offsetD{};
    SIMDF AoutL{}, AoutR{};
    SIMDF BoutL{}, BoutR{};
    SIMDF CoutL{}, CoutR{};
    SIMDF DoutL{}, DoutR{};

    for (int i = 0; i < numSamples; ++i)
    {
        la = A.out; lb = B.out; lc = C.out; ld = D.out;

        // compute mono outputs
        if constexpr (AOn)
            offsetA = la * A.feedback + lb * ba + lc * ca + ld * da;
        if constexpr (BOn)
            offsetB = la * ab + lb * B.feedback + lc * cb + ld * db;
        if constexpr (COn)
            offsetC = la * ac + lb * bc + lc * C.feedback + ld * dc;
        if constexpr (DOn)
            offsetD = la * ad + lb * bd + lc * cd + ld * D.feedback;

        // TODO saturate the feedback path
        //offsetA -= (offsetA * offsetA * offsetA) * 0.333
        // TODO scale feedback by frequency

        if constexpr (AOn)
            A.out = renderWave(table.data(), size, A.phase + offsetA) * A.level;
        if constexpr (BOn)
            B.out = renderWave(table.data(), size, B.phase + offsetB) * B.level;
        if constexpr (COn)
            C.out = renderWave(table.data(), size, C.phase + offsetC) * C.level;
        if constexpr (DOn)
            D.out = renderWave(table.data(), size, D.phase + offsetD) * D.level;

        if constexpr (AOn) AoutL = AoutR = A.out * AisOut;
        if constexpr (BOn) BoutL = BoutR = B.out * BisOut;
        if constexpr (COn) CoutL = CoutR = C.out * CisOut;
        if constexpr (DOn) DoutL = DoutR = D.out * DisOut;

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
        if constexpr (AOn) { A.phase += A.phase_inc; Utils::wrapPhase(A.phase); }
        if constexpr (BOn) { B.phase += B.phase_inc; Utils::wrapPhase(B.phase); }
        if constexpr (COn) { C.phase += C.phase_inc; Utils::wrapPhase(C.phase); }
        if constexpr (DOn) { D.phase += D.phase_inc; Utils::wrapPhase(D.phase); }

        // render output
        outL[i] = AoutL * A.gain_l + BoutL * B.gain_l + CoutL * C.gain_l + DoutL * D.gain_l;
        outR[i] = AoutR * A.gain_r + BoutR * B.gain_r + CoutR * C.gain_r + DoutR * D.gain_r;
    }
}


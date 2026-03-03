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
    morphAlpha = 1.f - std::exp(-1.f / (MORPH_SECONDS * srate));
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

static inline SIMDF renderWave(const std::array<float*, 8> tables, int size, SIMDF phase, SIMDF morph)
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

    SIMDF l1 = SIMDF{ tables[0][p[0]],      tables[1][p[1]],        tables[2][p[2]],        tables[3][p[3]] };
    SIMDF l2 = SIMDF{ tables[0][p[0] + 1],  tables[1][p[1] + 1],    tables[2][p[2] + 1],    tables[3][p[3] + 1] };

    if (morph.sum() > 0.f)
    {
        SIMDF l3 = SIMDF{ tables[4][p[0]],      tables[5][p[1]],        tables[6][p[2]],        tables[7][p[3]] };
        SIMDF l4 = SIMDF{ tables[4][p[0] + 1],  tables[5][p[1] + 1],    tables[6][p[2] + 1],    tables[7][p[3] + 1] };

        l1 += morph * (l3 - l1);
        l2 += morph * (l4 - l2);
    }

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

static inline bool hasCurrTableChanged(OSC::SIMDOSC& osc, FmMatrix::TablesData& tables)
{
    auto idx = (osc.morph * (tables.numTables - 1)).trunc();
    auto msk = ~(idx == tables.currIndex);
    return !msk.testz();
}

static inline SIMDF getMorph(OSC::SIMDOSC& osc, FmMatrix::TablesData& tables)
{
    auto tablepos = osc.morph * (tables.numTables - 1);
    return tablepos - tablepos.trunc();
}

template<bool AOn, bool BOn, bool COn, bool DOn>
void FmMatrix::_process(SIMDVox& vox, int numSamples)
{
    auto& A = vox.osc[0];
    auto& B = vox.osc[1];
    auto& C = vox.osc[2];
    auto& D = vox.osc[3];

    TablesData a_tables{};
    TablesData b_tables{};
    TablesData c_tables{};
    TablesData d_tables{};

    int a_tables_size = 0;
    int b_tables_size = 0;
    int c_tables_size = 0;
    int d_tables_size = 0;

    bool AisMorphing = AOn && (A.morph - A.morph_targ).abs().hmax() > 1e-4f;
    bool BisMorphing = BOn && (B.morph - B.morph_targ).abs().hmax() > 1e-4f;
    bool CisMorphing = COn && (C.morph - C.morph_targ).abs().hmax() > 1e-4f;
    bool DisMorphing = DOn && (D.morph - D.morph_targ).abs().hmax() > 1e-4f;

    if constexpr (AOn)
    {
        a_tables = getTables(vox, 0, AisMorphing);
        a_tables_size = audioProcessor.wavetables[0].tableSize;
    }

    if constexpr (BOn)
    {
        b_tables = getTables(vox, 1, BisMorphing);
        b_tables_size = audioProcessor.wavetables[1].tableSize;
    }

    if constexpr (COn)
    {
        c_tables = getTables(vox, 2, CisMorphing);
        c_tables_size = audioProcessor.wavetables[2].tableSize;
    }

    if constexpr (DOn)
    {
        d_tables = getTables(vox, 3, DisMorphing);
        d_tables_size = audioProcessor.wavetables[3].tableSize;
    }

    bool AhasUnison = AisOut && A.unison->voices > 1;
    bool BhasUnison = BisOut && B.unison->voices > 1;
    bool ChasUnison = CisOut && C.unison->voices > 1;
    bool DhasUnison = DisOut && D.unison->voices > 1;


    SIMDF la, lb, lc, ld;
    SIMDF offsetA(0.f), offsetB(0.f), offsetC(0.f), offsetD(0.f);
    SIMDF AoutL(0.f), AoutR(0.f);
    SIMDF BoutL(0.f), BoutR(0.f);
    SIMDF CoutL(0.f), CoutR(0.f);
    SIMDF DoutL(0.f), DoutR(0.f);

    SIMDF a_morph = getMorph(A, a_tables);
    SIMDF b_morph = getMorph(B, b_tables);
    SIMDF c_morph = getMorph(C, c_tables);
    SIMDF d_morph = getMorph(D, d_tables);

    for (int i = 0; i < numSamples; ++i)
    {
        la = A.out; lb = B.out; lc = C.out; ld = D.out;

        // compute fm offsets
        if constexpr (AOn)
            offsetA = la * A.feedback + lb * ba + lc * ca + ld * da;
        if constexpr (BOn)
            offsetB = la * ab + lb * B.feedback + lc * cb + ld * db;
        if constexpr (COn)
            offsetC = la * ac + lb * bc + lc * C.feedback + ld * dc;
        if constexpr (DOn)
            offsetD = la * ad + lb * bd + lc * cd + ld * D.feedback;

        // render mono outputs
        if constexpr (AOn)
            A.out = renderWave(a_tables.data, a_tables_size, A.phase + offsetA, a_morph) * A.level;
        if constexpr (BOn)
            B.out = renderWave(b_tables.data, b_tables_size, B.phase + offsetB, b_morph) * B.level;
        if constexpr (COn)
            C.out = renderWave(c_tables.data, c_tables_size, C.phase + offsetC, c_morph) * C.level;
        if constexpr (DOn)
            D.out = renderWave(d_tables.data, d_tables_size, D.phase + offsetD, d_morph) * D.level;

        if constexpr (AOn) AoutL = AoutR = A.out * AisOut;
        if constexpr (BOn) BoutL = BoutR = B.out * BisOut;
        if constexpr (COn) CoutL = CoutR = C.out * CisOut;
        if constexpr (DOn) DoutL = DoutR = D.out * DisOut;

        // render unison
        if constexpr (AOn)
        if (AhasUnison)
        {
            auto [uniL, uniR] = processUnison(A, offsetA);
            AoutL = uniL * A.level;
            AoutR = uniR * A.level;
        }

        if constexpr (BOn)
        if (BhasUnison)
        {
            auto [uniL, uniR] = processUnison(B, offsetB);
            BoutL = uniL * B.level;
            BoutR = uniR * B.level;
        }

        if constexpr (COn)
        if (ChasUnison)
        {
            auto [uniL, uniR] = processUnison(C, offsetC);
            CoutL = uniL * C.level;
            CoutR = uniR * C.level;
        }

        if constexpr (DOn)
        if (DhasUnison)
        {
            auto [uniL, uniR] = processUnison(D, offsetD);
            DoutL = uniL * D.level;
            DoutR = uniR * D.level;
        }

        // increment phases and interpolate

        if constexpr (AOn) 
        {
            if (AisMorphing) 
            {
                A.morph += (A.morph_targ - A.morph) * morphAlpha;
                a_morph = getMorph(A, a_tables);
                if (hasCurrTableChanged(A, a_tables))
                    a_tables = getTables(vox, 0, AisMorphing);
            }
            A.phase += A.phase_inc;
            Utils::wrapPhase(A.phase); 
        }

        if constexpr (BOn) 
        { 
            if (BisMorphing) 
            {
                B.morph += (B.morph_targ - B.morph) * morphAlpha;
                b_morph = getMorph(B, b_tables);
                if (hasCurrTableChanged(B, b_tables))
                    b_tables = getTables(vox, 1, BisMorphing);
            }
            B.phase += B.phase_inc; 
            Utils::wrapPhase(B.phase); 
        }

        if constexpr (COn) 
        { 
            if (CisMorphing) 
            {
                C.morph += (C.morph_targ - C.morph) * morphAlpha;
                c_morph = getMorph(C, c_tables);
                if (hasCurrTableChanged(C, c_tables))
                    c_tables = getTables(vox, 2, CisMorphing);
            }
            C.phase += C.phase_inc; 
            Utils::wrapPhase(C.phase); 
        }

        if constexpr (DOn) 
        {
            if (DisMorphing) 
            {
                D.morph += (D.morph_targ - D.morph) * morphAlpha;
                d_morph = getMorph(D, d_tables);
                if (hasCurrTableChanged(D, d_tables))
                    d_tables = getTables(vox, 3, DisMorphing);
            }
            D.phase += D.phase_inc; 
            Utils::wrapPhase(D.phase); 
        }

        // render output
        outL[i] = AoutL * A.gain_l + BoutL * B.gain_l + CoutL * C.gain_l + DoutL * D.gain_l;
        outR[i] = AoutR * A.gain_r + BoutR * B.gain_r + CoutR * C.gain_r + DoutR * D.gain_r;
    }
}

// fetches 1 table per voice + 1 morphing table per voice
FmMatrix::TablesData FmMatrix::getTables(SIMDVox& vox, int oscidx, bool isMorphing)
{
    alignas(sizeof(SIMDF)) std::array<float, 4> currIndex{};
    alignas(sizeof(SIMDF)) std::array<float, 4> targIndex{};

    TablesData out{};
    auto& tables = audioProcessor.wavetables[oscidx];
    out.numTables = tables.numTables;

    for (int i = 0; i < SIMD_SZ; ++i)
    {
        auto morph = vox.osc[oscidx].morph.get(i);
        auto tableIndex = int(float(tables.numTables - 1) * morph);
        auto t1 = tables.tables.getUnchecked(tableIndex);
        out.data[i] = t1->tableForNote(vox.voice.key[i]).data();
        currIndex[i] = tableIndex;

        int t2idx = i + SIMD_SZ;
        if (isMorphing && tableIndex < tables.numTables - 1)
        {
            auto morphtarg = vox.osc[oscidx].morph_targ.get(i);
            auto tableIdxTarg = std::min(tableIndex + 1, tables.numTables - 1);
            auto* t2 = tables.tables.getUnchecked(tableIdxTarg);
            out.data[t2idx] = t2->tableForNote(vox.voice.key[i]).data();
            targIndex[i] = tableIdxTarg;
        }
        else
        {
            out.data[t2idx] = out.data[i];
            targIndex[i] = tableIndex;
        }
    }

    out.currIndex.load(&currIndex[0]);
    out.targIndex.load(&targIndex[0]);

    return out;
}

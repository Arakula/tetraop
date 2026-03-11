#include "FmMatrix.h"
#include "../PluginProcessor.h"

FmMatrix::FmMatrix(TetraOPAudioProcessor& p) : audioProcessor(p)
{
    layouts[int(DCBA)][3][2] = 1.f; // DC
    layouts[int(DCBA)][2][1] = 1.f; // CB
    layouts[int(DCBA)][1][0] = 1.f; // BA

    layouts[int(DC_BA)][3][2] = 1.f; // DC
    layouts[int(DC_BA)][1][0] = 1.f; // BA

    layouts[int(DC_B_A)][3][2] = 1.f; // DC

    layouts[int(DA_DB_DC)][3][0] = 1.f; // DA
    layouts[int(DA_DB_DC)][3][1] = 1.f; // DB
    layouts[int(DA_DB_DC)][3][2] = 1.f; // DC

    layouts[int(BA_CA_DA)][1][0] = 1.f; // BA
    layouts[int(BA_CA_DA)][2][0] = 1.f; // CA
    layouts[int(BA_CA_DA)][3][0] = 1.f; // DA

    layouts[int(A_CB_DC)][2][1] = 1.f; // CB
    layouts[int(A_CB_DC)][3][2] = 1.f; // DC

    layouts[int(DC_CA_CB)][3][2] = 1.f; // DC
    layouts[int(DC_CA_CB)][2][0] = 1.f; // CA
    layouts[int(DC_CA_CB)][2][1] = 1.f; // CB

    layouts[int(DC_DB_BA_CA)][3][2] = 1.f; // DC
    layouts[int(DC_DB_BA_CA)][3][1] = 1.f; // DB
    layouts[int(DC_DB_BA_CA)][1][0] = 1.f; // BA
    layouts[int(DC_DB_BA_CA)][2][0] = 1.f; // CA

    layouts[int(DB_CB_BA)][3][1] = 1.f; // DB
    layouts[int(DB_CB_BA)][2][1] = 1.f; // CB
    layouts[int(DB_CB_BA)][1][0] = 1.f; // BA

    setLayout(Layout::DCBA);
}

FmMatrix::~FmMatrix()
{
}

void FmMatrix::parameterChanged(const juce::String&, float)
{
}

void FmMatrix::prepareDistortions(SIMDVox& vox)
{
    std::array<DistFn, 9> dists = {
        PhaseDist::bypass,
        PhaseDist::bend,
        PhaseDist::skew,
        PhaseDist::bias,
        PhaseDist::pulse,
        PhaseDist::sync,
        PhaseDist::sync,
        PhaseDist::quantize,
        PhaseDist::fold
    };

    auto adist = (PhaseDist::Mode)audioProcessor.params.getRawParameterValue("a_phase_dist_mode")->load();
    auto bdist = (PhaseDist::Mode)audioProcessor.params.getRawParameterValue("b_phase_dist_mode")->load();
    auto cdist = (PhaseDist::Mode)audioProcessor.params.getRawParameterValue("c_phase_dist_mode")->load();
    auto ddist = (PhaseDist::Mode)audioProcessor.params.getRawParameterValue("d_phase_dist_mode")->load();

    Adist = Utils::allLanesZero(vox.osc[0].dist_amt) ? dists[0] : dists[adist];
    Bdist = Utils::allLanesZero(vox.osc[1].dist_amt) ? dists[0] : dists[adist];
    Cdist = Utils::allLanesZero(vox.osc[2].dist_amt) ? dists[0] : dists[adist];
    Ddist = Utils::allLanesZero(vox.osc[3].dist_amt) ? dists[0] : dists[adist];

    // optimize bend distortion (heavy) by selecting only positive or negative paths
    if (Adist == dists[1])
        if (Utils::allLanesPositiveOrZero(vox.osc[0].dist_amt)) Adist = PhaseDist::bendPos;
        else if (Utils::allLanesNegativeOrZero(vox.osc[0].dist_amt)) Adist = PhaseDist::bendNeg;

    if (Bdist == dists[1])
        if (Utils::allLanesPositiveOrZero(vox.osc[1].dist_amt)) Bdist = PhaseDist::bendPos;
        else if (Utils::allLanesNegativeOrZero(vox.osc[1].dist_amt)) Bdist = PhaseDist::bendNeg;

    if (Cdist == dists[1])
        if (Utils::allLanesPositiveOrZero(vox.osc[2].dist_amt)) Cdist = PhaseDist::bendPos;
        else if (Utils::allLanesNegativeOrZero(vox.osc[2].dist_amt)) Cdist = PhaseDist::bendNeg;

    if (Ddist == dists[1])
        if (Utils::allLanesPositiveOrZero(vox.osc[3].dist_amt)) Ddist = PhaseDist::bendPos;
        else if (Utils::allLanesNegativeOrZero(vox.osc[3].dist_amt)) Ddist = PhaseDist::bendNeg;

    Awindow = adist == 6 ? PhaseDist::windowHalfSine : PhaseDist::windowBypass;
    Bwindow = bdist == 6 ? PhaseDist::windowHalfSine : PhaseDist::windowBypass;
    Cwindow = cdist == 6 ? PhaseDist::windowHalfSine : PhaseDist::windowBypass;
    Dwindow = ddist == 6 ? PhaseDist::windowHalfSine : PhaseDist::windowBypass;
}

void FmMatrix::setLayout(Layout l)
{
    layout = l;
    matrix = layouts[layout];

    ab = matrix[0][1]; ac = matrix[0][2]; ad = matrix[0][3];
    ba = matrix[1][0]; bc = matrix[1][2]; bd = matrix[1][3];
    ca = matrix[2][0]; cb = matrix[2][1]; cd = matrix[2][3];
    da = matrix[3][0]; db = matrix[3][1]; dc = matrix[3][2];

    auto isoutput = [this](int row) {
        return !matrix[row][0] && !matrix[row][1] && !matrix[row][2] && !matrix[row][3];
    };

    AisOut = isoutput(0) ? 1.f : 0.f; isOut[0] = AisOut;
    BisOut = isoutput(1) ? 1.f : 0.f; isOut[1] = BisOut;
    CisOut = isoutput(2) ? 1.f : 0.f; isOut[2] = CisOut;
    DisOut = isoutput(3) ? 1.f : 0.f; isOut[3] = DisOut;
}

void FmMatrix::prepare(float _srate)
{
    srate = _srate;
    morphAlpha = 1.f - std::exp(-1.f / (MORPH_SECONDS * srate));
}

static inline SIMDF renderUnison(const float* table1, const float* table2, const int size, SIMDF phase, float morph)
{
    static constexpr float almostOne = 1.f - std::numeric_limits<float>::epsilon();

    Utils::wrapPhase(phase);
    auto posf = phase * float(size);
    auto posi = posf.trunc();
    auto t = posf - posi;
    auto t2 = t * t;
    auto t3 = t2 * t;
    auto posint = mipp::cvt<float, int32_t>(posi) + 1; // +1 because tables are padded

    SIMDF y0 = mipp::gather(&table1[0], posint - 1);
    SIMDF y1 = mipp::gather(&table1[0], posint + 0);
    SIMDF y2 = mipp::gather(&table1[0], posint + 1);
    SIMDF y3 = mipp::gather(&table1[0], posint + 2);

    if (morph > 0.f)
    {
        SIMDF y02 = mipp::gather(&table2[0], posint - 1);
        SIMDF y12 = mipp::gather(&table2[0], posint);
        SIMDF y22 = mipp::gather(&table2[0], posint + 1);
        SIMDF y32 = mipp::gather(&table2[0], posint + 2);

        auto smorph = SIMDF(morph);
        y0 = mipp::fmadd(smorph, (y02 - y0), y0);
        y1 = mipp::fmadd(smorph, (y12 - y1), y1);
        y2 = mipp::fmadd(smorph, (y22 - y2), y2);
        y3 = mipp::fmadd(smorph, (y32 - y3), y3);
    }

    auto c1 = y0.fmadd(-0.5f, y1.fmadd(1.5f, (-y2).fmadd(1.5f, y3 * 0.5f)));
    auto c2 = y0 + (-y1).fmadd(2.5f, y2.fmadd(2.f, -y3 * 0.5f));
    auto c3 = y0.fmadd(-0.5f, y2 * 0.5f);
    return c1.fmadd(t3, c2.fmadd(t2, c3.fmadd(t, y1)));
}

static inline std::pair<SIMDF, SIMDF> processUnison(
    OSC::SIMDOSC& osc, 
    SIMDF phaseOffset,
    std::array<float*, 8>& tables,
    int size,
    SIMDF morph,
    DistFn dist,
    WindowFn window,
    SIMDF voiceMask
)
{
    alignas(sizeof(SIMDF)) float accL[4] = { 0.f, 0.f, 0.f, 0.f };
    alignas(sizeof(SIMDF)) float accR[4] = { 0.f, 0.f, 0.f, 0.f };

    for (int lane = 0; lane < 4; ++lane) // for each voice
    {
        if (!voiceMask.get(lane)) continue;
        auto& U = osc.unison[lane];
        const int batch = (U.voices + 3) >> 2;
        const auto offset = SIMDF(phaseOffset.get(lane) + osc.phase_offset.get(lane));
        const int t2lane = lane + 4;
        const float pitch_ratio = osc.pitch_ratio.get(lane);

        SIMDF phs;
        for (int v = 0; v < batch; ++v) // for each unison voice
        {
            phs = U.phase[v] + offset;
            SIMDF s = renderUnison(tables[lane], tables[t2lane], size, dist(phs, osc.dist_amt), morph.get(lane));
            window(s, phs);
            s *= U.mask[v];

            accL[lane] += (s * U.gain_l[v]).sum();
            accR[lane] += (s * U.gain_r[v]).sum();

            auto& phase = U.phase[v];
            phase = U.inc[v].fmadd(pitch_ratio, phase);
            Utils::wrapPhase(U.phase[v]);
        }
    }

    return { mipp::load(accL), mipp::load(accR) };
}

// SIMD'ed voices rendering
void FmMatrix::processBlock(SIMDVox& vox, int numSamples, int activeVoice, SIMDF vmask)
{
    prepareDistortions(vox);

    bool aon = vox.osc[0].level.hmax() > 1e-5 || vox.osc[0].level_targ.hmax() > 1e-5;
    bool bon = vox.osc[1].level.hmax() > 1e-5 || vox.osc[1].level_targ.hmax() > 1e-5;
    bool con = vox.osc[2].level.hmax() > 1e-5 || vox.osc[2].level_targ.hmax() > 1e-5;
    bool don = vox.osc[3].level.hmax() > 1e-5 || vox.osc[3].level_targ.hmax() > 1e-5;

    int mask = (aon << 3) | (bon << 2) | (con << 1) | (don << 0);

    switch (mask)
    {
    case 0b0000: _process<false, false, false, false>(vox, numSamples, activeVoice, vmask); break;
    case 0b0001: _process<false, false, false, true>(vox, numSamples, activeVoice, vmask); break;
    case 0b0010: _process<false, false, true, false>(vox, numSamples, activeVoice, vmask); break;
    case 0b0011: _process<false, false, true, true>(vox, numSamples, activeVoice, vmask); break;
    case 0b0100: _process<false, true, false, false>(vox, numSamples, activeVoice, vmask); break;
    case 0b0101: _process<false, true, false, true>(vox, numSamples, activeVoice, vmask); break;
    case 0b0110: _process<false, true, true, false>(vox, numSamples, activeVoice, vmask); break;
    case 0b0111: _process<false, true, true, true>(vox, numSamples, activeVoice, vmask); break;
    case 0b1000: _process<true, false, false, false>(vox, numSamples, activeVoice, vmask); break;
    case 0b1001: _process<true, false, false, true>(vox, numSamples, activeVoice, vmask); break;
    case 0b1010: _process<true, false, true, false>(vox, numSamples, activeVoice, vmask); break;
    case 0b1011: _process<true, false, true, true>(vox, numSamples, activeVoice, vmask); break;
    case 0b1100: _process<true, true, false, false>(vox, numSamples, activeVoice, vmask); break;
    case 0b1101: _process<true, true, false, true>(vox, numSamples, activeVoice, vmask); break;
    case 0b1110: _process<true, true, true, false>(vox, numSamples, activeVoice, vmask); break;
    case 0b1111: _process<true, true, true, true>(vox, numSamples, activeVoice, vmask); break;
    }
}

// checks if global morph position is at a different index from tables.currIndex
static inline bool hasCurrTableChanged(OSC::SIMDOSC& osc, FmMatrix::TablesData& tables)
{
    float tablesf = static_cast<float>(tables.numTables);
    auto idx = (osc.morph * tablesf).trunc().min(tablesf - 1.f);
    auto msk = ~(idx == tables.currIndex);
    return !msk.testz();
}

static inline SIMDF getMorph(OSC::SIMDOSC& osc, FmMatrix::TablesData& tables)
{
    float tablesf = static_cast<float>(tables.numTables);
    auto tablepos = (osc.morph * tablesf).min(tablesf - 1.f);
    return (tablepos - tablepos.trunc()) * tables.isMorphing;
}

template<bool AOn, bool BOn, bool COn, bool DOn>
void FmMatrix::_process(SIMDVox& vox, int numSamples, const int activeVoice, SIMDF vmask)
{
    auto& A = vox.osc[0];
    auto& B = vox.osc[1];
    auto& C = vox.osc[2];
    auto& D = vox.osc[3];

    TablesData a_tables{};
    TablesData b_tables{};
    TablesData c_tables{};
    TablesData d_tables{};

    const bool AisMorphing = AOn && (A.morph - A.morph_targ).abs().hmax() > 1e-4f;
    const bool BisMorphing = BOn && (B.morph - B.morph_targ).abs().hmax() > 1e-4f;
    const bool CisMorphing = COn && (C.morph - C.morph_targ).abs().hmax() > 1e-4f;
    const bool DisMorphing = DOn && (D.morph - D.morph_targ).abs().hmax() > 1e-4f;

    auto isamps = 1.f / numSamples;
    if constexpr (AOn)
    {
        a_tables = getTables(vox, 0, AisMorphing);
        A.level_step = (A.level_targ - A.level) * isamps;
        A.pitch_ratio_step = (A.pitch_ratio_targ - A.pitch_ratio) * isamps;
    }

    if constexpr (BOn)
    {
        b_tables = getTables(vox, 1, BisMorphing);
        B.level_step = (B.level_targ - B.level) * isamps;
        B.pitch_ratio_step = (B.pitch_ratio_targ - B.pitch_ratio) * isamps;
    }

    if constexpr (COn)
    {
        c_tables = getTables(vox, 2, CisMorphing);
        C.level_step = (C.level_targ - C.level) * isamps;
        C.pitch_ratio_step = (C.pitch_ratio_targ - C.pitch_ratio) * isamps;
    }

    if constexpr (DOn)
    {
        d_tables = getTables(vox, 3, DisMorphing);
        D.level_step = (D.level_targ - D.level) * isamps;
        D.pitch_ratio_step = (D.pitch_ratio_targ - D.pitch_ratio) * isamps;
    }

    const bool AisNoise = AOn && (a_tables.isWhiteNoise || a_tables.isPinkNoise);
    const bool BisNoise = BOn && (b_tables.isWhiteNoise || b_tables.isPinkNoise);
    const bool CisNoise = COn && (c_tables.isWhiteNoise || c_tables.isPinkNoise);
    const bool DisNoise = DOn && (d_tables.isWhiteNoise || d_tables.isPinkNoise);

    static auto hasUnisonVoices = [](const OSC::SIMDUnison* uni, const SIMDF& mask)
        {
            for (int i = 0; i < SIMDSZ; ++i)
                if (uni[i].voices > 1 && mask.get(i) > 0.f)
                    return true;

            return false;
        };

    const bool AhasUnison = AOn && AisOut.hmax() > 0.f && hasUnisonVoices(A.unison, vmask) && !AisNoise;
    const bool BhasUnison = BOn && BisOut.hmax() > 0.f && hasUnisonVoices(B.unison, vmask) && !BisNoise;
    const bool ChasUnison = COn && CisOut.hmax() > 0.f && hasUnisonVoices(C.unison, vmask) && !CisNoise;
    const bool DhasUnison = DOn && DisOut.hmax() > 0.f && hasUnisonVoices(D.unison, vmask) && !DisNoise;

    const RenderFn renderA = a_tables.isWhiteNoise ? renderWhiteNoise : a_tables.isPinkNoise ? renderPinkNoise 
        : AisOut.hmax() > 0.f ? renderWaveLinear : renderWaveLinear;
    const RenderFn renderB = b_tables.isWhiteNoise ? renderWhiteNoise : b_tables.isPinkNoise ? renderPinkNoise
        : BisOut.hmax() > 0.f ? renderWaveLinear : renderWaveLinear;
    const RenderFn renderC = c_tables.isWhiteNoise ? renderWhiteNoise : c_tables.isPinkNoise ? renderPinkNoise
        : CisOut.hmax() > 0.f ? renderWaveLinear : renderWaveLinear;
    const RenderFn renderD = d_tables.isWhiteNoise ? renderWhiteNoise : d_tables.isPinkNoise ? renderPinkNoise
        : DisOut.hmax() > 0.f ? renderWaveLinear : renderWaveLinear;

    // temp vars
    SIMDF la, lb, lc, ld, fm_phase; 
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
            offsetA = la.fmadd(A.feedback, lb.fmadd(ba, lc.fmadd(ca, ld * da)));
        if constexpr (BOn)
            offsetB = la.fmadd(ab, lb.fmadd(B.feedback, lc.fmadd(cb, ld * db)));
        if constexpr (COn)
            offsetC = la.fmadd(ac, lb.fmadd(bc, lc.fmadd(C.feedback, ld * dc)));
        if constexpr (DOn)
            offsetD = la.fmadd(ad, lb.fmadd(bd, lc.fmadd(cd, ld * D.feedback)));

        // render mono outputs
        if constexpr (AOn) 
        {
            fm_phase = A.phase + A.phase_offset + offsetA;
            Utils::wrapPhase(fm_phase);
            A.out = renderA(a_tables.data, a_tables.size, Adist(fm_phase, A.dist_amt), a_morph, A) * A.level;
            Awindow(A.out, fm_phase); // apply window function (formant distortion only)
        }
        if constexpr (BOn)
        {
            fm_phase = B.phase + B.phase_offset + offsetB;
            Utils::wrapPhase(fm_phase);
            B.out = renderB(b_tables.data, b_tables.size, Bdist(fm_phase, B.dist_amt), b_morph, B) * B.level;
            Bwindow(B.out, fm_phase);
        }
        if constexpr (COn)
        {
            fm_phase = C.phase + C.phase_offset + offsetC;
            Utils::wrapPhase(fm_phase);
            C.out = renderC(c_tables.data, c_tables.size, Cdist(fm_phase, C.dist_amt), c_morph, C) * C.level;
            Cwindow(C.out, fm_phase);
        }
        if constexpr (DOn)
        {
            fm_phase = D.phase + D.phase_offset + offsetD;
            Utils::wrapPhase(fm_phase);
            D.out = renderD(d_tables.data, d_tables.size, Ddist(fm_phase, D.dist_amt), d_morph, D) * D.level;
            Dwindow(D.out, D.phase);
        }

        if constexpr (AOn) AoutL = AoutR = A.out;
        if constexpr (BOn) BoutL = BoutR = B.out;
        if constexpr (COn) CoutL = CoutR = C.out;
        if constexpr (DOn) DoutL = DoutR = D.out;

        // render unison
        if constexpr (AOn)
        if (AhasUnison)
        {
            auto [uniL, uniR] = processUnison(A, offsetA, a_tables.data, a_tables.size, a_morph, Adist, Awindow, vmask);
            AoutL = uniL * A.level;
            AoutR = uniR * A.level;
        }

        if constexpr (BOn)
        if (BhasUnison)
        {
            auto [uniL, uniR] = processUnison(B, offsetB, b_tables.data, b_tables.size, b_morph, Bdist, Bwindow, vmask);
            BoutL = uniL * B.level;
            BoutR = uniR * B.level;
        }

        if constexpr (COn)
        if (ChasUnison)
        {
            auto [uniL, uniR] = processUnison(C, offsetC, c_tables.data, c_tables.size, c_morph, Cdist, Cwindow, vmask);
            CoutL = uniL * C.level;
            CoutR = uniR * C.level;
        }

        if constexpr (DOn)
        if (DhasUnison)
        {
            auto [uniL, uniR] = processUnison(D, offsetD, d_tables.data, d_tables.size, d_morph, Ddist, Dwindow, vmask);
            DoutL = uniL * D.level;
            DoutR = uniR * D.level;
        }

        // increment phases and interpolate
        if constexpr (AOn) 
        {
            if (AisMorphing)
            {
                A.morph = (A.morph_targ - A.morph).fmadd(morphAlpha, A.morph);
                a_morph = getMorph(A, a_tables);
                if (hasCurrTableChanged(A, a_tables))
                    a_tables = getTables(vox, 0, AisMorphing);
            }
            A.level += A.level_step;
            A.phase = A.phase_inc.fmadd(A.pitch_ratio, A.phase);
            A.pitch_ratio += A.pitch_ratio_step;
            Utils::wrapPhase(A.phase); 
        }
        if constexpr (BOn) 
        { 
            if (BisMorphing) 
            {
                B.morph = (B.morph_targ - B.morph).fmadd(morphAlpha, B.morph);
                b_morph = getMorph(B, b_tables);
                if (hasCurrTableChanged(B, b_tables))
                    b_tables = getTables(vox, 1, BisMorphing);
            }
            B.level += B.level_step;
            B.phase = B.phase_inc.fmadd(B.pitch_ratio, B.phase);
            B.pitch_ratio += B.pitch_ratio_step;
            Utils::wrapPhase(B.phase); 
        }
        if constexpr (COn) 
        { 
            if (CisMorphing) 
            {
                C.morph = (C.morph_targ - C.morph).fmadd(morphAlpha, C.morph);
                c_morph = getMorph(C, c_tables);
                if (hasCurrTableChanged(C, c_tables))
                    c_tables = getTables(vox, 2, CisMorphing);
            }
            C.level += C.level_step;
            C.phase = C.phase_inc.fmadd(C.pitch_ratio, C.phase);
            C.pitch_ratio += C.pitch_ratio_step;
            Utils::wrapPhase(C.phase); 
        }
        if constexpr (DOn) 
        {
            if (DisMorphing) 
            {
                D.morph = (D.morph_targ - D.morph).fmadd(morphAlpha, D.morph);
                d_morph = getMorph(D, d_tables);
                if (hasCurrTableChanged(D, d_tables))
                    d_tables = getTables(vox, 3, DisMorphing);
            }
            D.level += D.level_step;
            D.phase = D.phase_inc.fmadd(D.pitch_ratio, D.phase);
            D.pitch_ratio += D.pitch_ratio_step;
            Utils::wrapPhase(D.phase); 
        }

        // sample outputs for UI oscilloscopes
        if (activeVoice > -1)
        {
            if constexpr (AOn) sampleOscilloscope(A, AoutL, AoutR, 0, activeVoice);
            if constexpr (BOn) sampleOscilloscope(B, BoutL, BoutR, 1, activeVoice);
            if constexpr (COn) sampleOscilloscope(C, CoutL, CoutR, 2, activeVoice);
            if constexpr (DOn) sampleOscilloscope(D, DoutL, DoutR, 3, activeVoice);
        }

        // scale and filter outputs
        if constexpr (AOn) { AoutL *= AisOut * vmask; AoutR *= AisOut * vmask; }
        if constexpr (BOn) { BoutL *= BisOut * vmask; BoutR *= BisOut * vmask; }
        if constexpr (COn) { CoutL *= CisOut * vmask; CoutR *= CisOut * vmask; }
        if constexpr (DOn) { DoutL *= DisOut * vmask; DoutR *= DisOut * vmask; }

        // render stereo output
        outL[0][i] = AoutL * A.gain_l; 
        outL[1][i] = BoutL * B.gain_l;
        outL[2][i] = CoutL * C.gain_l;
        outL[3][i] = DoutL * D.gain_l;
        outR[0][i] = AoutR * A.gain_r;
        outR[1][i] = BoutR * B.gain_r;
        outR[2][i] = CoutR * C.gain_r;
        outR[3][i] = DoutR * D.gain_r;
    }

    // finish block

    if constexpr (AOn)
    {
        A.level = A.level_targ;
        A.pitch_ratio = A.pitch_ratio_targ;
    }
    if constexpr (BOn)
    {
        B.level = B.level_targ;
        B.pitch_ratio = B.pitch_ratio_targ;
    }
    if constexpr (COn)
    {
        C.level = C.level_targ;
        C.pitch_ratio = C.pitch_ratio_targ;
    }
    if constexpr (DOn)
    {
        D.level = D.level_targ;
        D.pitch_ratio = D.pitch_ratio_targ;
    }

}

// fetches 1 table per voice + 1 morphing table per voice
FmMatrix::TablesData FmMatrix::getTables(SIMDVox& vox, int oscId, bool isMorphing)
{
    alignas(sizeof(SIMDF)) std::array<float, 4> currIndex{};
    alignas(sizeof(SIMDF)) std::array<float, 4> targIndex{};

    TablesData out{};
    auto& tables = audioProcessor.wavetables[oscId];
    out.numTables = tables.numTables;
    out.size = tables.tableSize;
    out.isWhiteNoise = (int)tables.mode == TetraOPAudioProcessor::WTMode::WhiteNoise;
    out.isPinkNoise = (int)tables.mode == TetraOPAudioProcessor::WTMode::PinkNoise;
    out.isMorphing = isMorphing;

    for (int i = 0; i < SIMDSZ; ++i) // for each voice get OSC wavetables
    {
        auto morph = vox.osc[oscId].morph.get(i);
        auto tableIndex = std::min(tables.numTables - 1, int(float(tables.numTables) * morph));
        auto* t1 = tables.tables.getUnchecked(tableIndex);
        out.data[i] = t1->tableForNote(vox.voice.key[i]).data();
        currIndex[i] = (float)tableIndex;

        int t2idx = i + SIMDSZ;
        if (isMorphing && tableIndex < tables.numTables - 1)
        {
            auto tableIdxTarg = std::min(tableIndex + 1, tables.numTables - 1);
            auto* t2 = tables.tables.getUnchecked(tableIdxTarg);
            out.data[t2idx] = t2->tableForNote(vox.voice.key[i]).data();
            targIndex[i] = (float)tableIdxTarg;
        }
        else
        {
            out.data[t2idx] = out.data[i];
            targIndex[i] = (float)tableIndex;
        }
    }

    out.currIndex.load(&currIndex[0]);
    out.targIndex.load(&targIndex[0]);

    return out;
}

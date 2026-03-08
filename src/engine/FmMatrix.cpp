#include "FmMatrix.h"
#include "../PluginProcessor.h"

FmMatrix::FmMatrix(TetraOPAudioProcessor& p) : audioProcessor(p)
{
    audioProcessor.params.addParameterListener("a_phase_dist_mode", this);
    audioProcessor.params.addParameterListener("b_phase_dist_mode", this);
    audioProcessor.params.addParameterListener("c_phase_dist_mode", this);
    audioProcessor.params.addParameterListener("d_phase_dist_mode", this);

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
    audioProcessor.params.removeParameterListener("a_phase_dist_mode", this);
    audioProcessor.params.removeParameterListener("b_phase_dist_mode", this);
    audioProcessor.params.removeParameterListener("c_phase_dist_mode", this);
    audioProcessor.params.removeParameterListener("d_phase_dist_mode", this);
}

void FmMatrix::parameterChanged(const juce::String&, float)
{
    paramsChanged = true;
}

void FmMatrix::onParamsChange()
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

    Adist = dists[adist];
    Bdist = dists[bdist];
    Cdist = dists[cdist];
    Ddist = dists[ddist];

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

bool FmMatrix::isNoise(int oscId)
{
    auto& wt = audioProcessor.wavetables[oscId];
    return wt.mode == TetraOPAudioProcessor::WhiteNoise || wt.mode == TetraOPAudioProcessor::PinkNoise;
}

void FmMatrix::fetchNoiseGenerators(int oscId, SIMDI voiceId)
{
    auto& wt = audioProcessor.wavetables[oscId];
    bool isWhiteNoise = wt.mode == TetraOPAudioProcessor::WhiteNoise;

    for (int i = 0; i < SIMD_SZ; ++i) 
    {
        auto* voice = (Voice*)audioProcessor.synth->getVoice(voiceId.get(i));
        if (!voice) continue;
        auto& osc = voice->osc[oscId];
        auto& ng = noiseGens[oscId];
        ng[i] = isWhiteNoise ? (NoiseGen*)&osc.noiseGen : (NoiseGen*)&osc.pinkNoiseGen;
    }
}


static inline SIMDF renderUnison(const float* table1, const float* table2, const int size, SIMDF phase, float morph)
{
    static constexpr float almostOne = 1.f - std::numeric_limits<float>::epsilon();

    Utils::wrapPhase(phase);
    auto posf = phase * float(size);
    auto posi = posf.trunc();
    auto t = posf - posi;
    auto posint = mipp::cvt<float, int32_t>(posi) + 1; // +1 because tables are padded

    SIMDF l1 = mipp::gather(&table1[0], posint);
    SIMDF l2 = mipp::gather(&table1[0], posint + 1);

    if (morph > 0.f)
    {
        SIMDF l3 = mipp::gather(&table2[0], posint);
        SIMDF l4 = mipp::gather(&table2[0], posint + 1);

        l1 = mipp::fmadd(SIMDF(morph), (l3 - l1), l1);
        l2 = mipp::fmadd(SIMDF(morph), (l4 - l2), l2);
    }

    return mipp::fmadd(t, (l2 - l1), l1);
}

static inline std::pair<SIMDF, SIMDF> processUnison(
    OSC::SIMDOSC& osc, 
    SIMDF phaseOffset,
    std::array<float*, 8>& tables,
    int size,
    SIMDF morph
)
{
    alignas(sizeof(SIMDF)) float accL[4] = { 0.f, 0.f, 0.f, 0.f };
    alignas(sizeof(SIMDF)) float accR[4] = { 0.f, 0.f, 0.f, 0.f };

    for (int lane = 0; lane < 4; ++lane) // for each voice
    {
        auto& U = osc.unison[lane];
        const int batch = (U.voices + 3) >> 2;
        const auto offset = SIMDF(phaseOffset.get(lane) + osc.phase_offset.get(lane));
        const int t2lane = lane + 4;
        const float pitch_ratio = osc.pitch_ratio.get(lane);

        for (int v = 0; v < batch; ++v) // for each unison voice
        {
            SIMDF s = renderUnison(tables[lane], tables[t2lane], size, U.phase[v] + offset, morph.get(lane));
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
void FmMatrix::processBlock(SIMDVox& data, int numSamples, int activeVoice)
{
    if (paramsChanged)
    {
        onParamsChange();
        paramsChanged = false;
    }

    bool aon = data.osc[0].level.sum() > 1e-5 || data.osc[0].level_step.sum() > 1e-5;
    bool bon = data.osc[1].level.sum() > 1e-5 || data.osc[1].level_step.sum() > 1e-5;
    bool con = data.osc[2].level.sum() > 1e-5 || data.osc[2].level_step.sum() > 1e-5;
    bool don = data.osc[3].level.sum() > 1e-5 || data.osc[3].level_step.sum() > 1e-5;

    int mask = (aon << 3) | (bon << 2) | (con << 1) | (don << 0);

    switch (mask)
    {
    case 0b0000: _process<false, false, false, false>(data, numSamples, activeVoice); break;
    case 0b0001: _process<false, false, false, true>(data, numSamples, activeVoice); break;
    case 0b0010: _process<false, false, true, false>(data, numSamples, activeVoice); break;
    case 0b0011: _process<false, false, true, true>(data, numSamples, activeVoice); break;
    case 0b0100: _process<false, true, false, false>(data, numSamples, activeVoice); break;
    case 0b0101: _process<false, true, false, true>(data, numSamples, activeVoice); break;
    case 0b0110: _process<false, true, true, false>(data, numSamples, activeVoice); break;
    case 0b0111: _process<false, true, true, true>(data, numSamples, activeVoice); break;
    case 0b1000: _process<true, false, false, false>(data, numSamples, activeVoice); break;
    case 0b1001: _process<true, false, false, true>(data, numSamples, activeVoice); break;
    case 0b1010: _process<true, false, true, false>(data, numSamples, activeVoice); break;
    case 0b1011: _process<true, false, true, true>(data, numSamples, activeVoice); break;
    case 0b1100: _process<true, true, false, false>(data, numSamples, activeVoice); break;
    case 0b1101: _process<true, true, false, true>(data, numSamples, activeVoice); break;
    case 0b1110: _process<true, true, true, false>(data, numSamples, activeVoice); break;
    case 0b1111: _process<true, true, true, true>(data, numSamples, activeVoice); break;
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
    return tablepos - tablepos.trunc();
}

template<bool AOn, bool BOn, bool COn, bool DOn>
void FmMatrix::_process(SIMDVox& vox, int numSamples, const int activeVoice)
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

    if constexpr (AOn) a_tables = getTables(vox, 0, AisMorphing);
    if constexpr (BOn) b_tables = getTables(vox, 1, BisMorphing);
    if constexpr (COn) c_tables = getTables(vox, 2, CisMorphing);
    if constexpr (DOn) d_tables = getTables(vox, 3, DisMorphing);

    const bool AisNoise = AOn && isNoise(0);
    const bool BisNoise = BOn && isNoise(1);
    const bool CisNoise = COn && isNoise(2);
    const bool DisNoise = DOn && isNoise(3);

    const bool AhasUnison = AisOut && A.unison->voices > 1 && !AisNoise;
    const bool BhasUnison = BisOut && B.unison->voices > 1 && !BisNoise;
    const bool ChasUnison = CisOut && C.unison->voices > 1 && !CisNoise;
    const bool DhasUnison = DisOut && D.unison->voices > 1 && !DisNoise;

    std::array<NoiseGen*, 4>* ANoiseGen = nullptr;
    std::array<NoiseGen*, 4>* BNoiseGen = nullptr;
    std::array<NoiseGen*, 4>* CNoiseGen = nullptr;
    std::array<NoiseGen*, 4>* DNoiseGen = nullptr;
    if (AisNoise) { fetchNoiseGenerators(0, vox.voice.id); ANoiseGen = &noiseGens[0]; };
    if (BisNoise) { fetchNoiseGenerators(1, vox.voice.id); BNoiseGen = &noiseGens[1]; };
    if (CisNoise) { fetchNoiseGenerators(2, vox.voice.id); CNoiseGen = &noiseGens[2]; };
    if (DisNoise) { fetchNoiseGenerators(3, vox.voice.id); DNoiseGen = &noiseGens[3]; };

    const RenderFn renderA = isNoise(0) ? renderNoise : AisOut ? renderWaveCubic : renderWaveLinear;
    const RenderFn renderB = isNoise(1) ? renderNoise : BisOut ? renderWaveCubic : renderWaveLinear;
    const RenderFn renderC = isNoise(2) ? renderNoise : CisOut ? renderWaveCubic : renderWaveLinear;
    const RenderFn renderD = isNoise(3) ? renderNoise : DisOut ? renderWaveCubic : renderWaveLinear;

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
            A.out = renderA(a_tables.data, a_tables.size, Adist(fm_phase, A.dist_amt), a_morph, ANoiseGen) * A.level;
            Awindow(A.out, fm_phase);
        }
        if constexpr (BOn)
        {
            fm_phase = B.phase + B.phase_offset + offsetB;
            Utils::wrapPhase(fm_phase);
            B.out = renderB(b_tables.data, b_tables.size, Bdist(fm_phase, B.dist_amt), b_morph, BNoiseGen) * B.level;
            Bwindow(B.out, fm_phase);
        }
        if constexpr (COn)
        {
            fm_phase = C.phase + C.phase_offset + offsetC;
            Utils::wrapPhase(fm_phase);
            C.out = renderC(c_tables.data, c_tables.size, Cdist(fm_phase, C.dist_amt), c_morph, CNoiseGen) * C.level;
            Cwindow(C.out, fm_phase);
        }
        if constexpr (DOn)
        {
            fm_phase = D.phase + D.phase_offset + offsetD;
            Utils::wrapPhase(fm_phase);
            D.out = renderD(d_tables.data, d_tables.size, Ddist(fm_phase, D.dist_amt), d_morph, DNoiseGen) * D.level;
            Dwindow(D.out, D.phase)
        }

        if constexpr (AOn) AoutL = AoutR = A.out;
        if constexpr (BOn) BoutL = BoutR = B.out;
        if constexpr (COn) CoutL = CoutR = C.out;
        if constexpr (DOn) DoutL = DoutR = D.out;

        // render unison
        if constexpr (AOn)
        if (AhasUnison)
        {
            auto [uniL, uniR] = processUnison(A, offsetA, a_tables.data, a_tables.size, a_morph);
            AoutL = uniL * A.level;
            AoutR = uniR * A.level;
        }

        if constexpr (BOn)
        if (BhasUnison)
        {
            auto [uniL, uniR] = processUnison(B, offsetB, b_tables.data, b_tables.size, b_morph);
            BoutL = uniL * B.level;
            BoutR = uniR * B.level;
        }

        if constexpr (COn)
        if (ChasUnison)
        {
            auto [uniL, uniR] = processUnison(C, offsetC, c_tables.data, c_tables.size, c_morph);
            CoutL = uniL * C.level;
            CoutR = uniR * C.level;
        }

        if constexpr (DOn)
        if (DhasUnison)
        {
            auto [uniL, uniR] = processUnison(D, offsetD, d_tables.data, d_tables.size, d_morph);
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

        // scale outputs by FM matrix outputs
        if constexpr (AOn) { AoutL *= AisOut; AoutR *= AisOut; }
        if constexpr (BOn) { BoutL *= BisOut; BoutR *= BisOut; }
        if constexpr (COn) { CoutL *= CisOut; CoutR *= CisOut; }
        if constexpr (DOn) { DoutL *= DisOut; DoutR *= DisOut; }

        // render stereo output
        outL[i] = AoutL.fmadd(A.gain_l, BoutL.fmadd(B.gain_l, CoutL.fmadd(C.gain_l, DoutL * D.gain_l)));
        outR[i] = AoutR.fmadd(A.gain_r, BoutR.fmadd(B.gain_r, CoutR.fmadd(C.gain_r, DoutR * D.gain_r)));
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

    for (int i = 0; i < SIMD_SZ; ++i) // for each voice get OSC wavetables
    {
        auto morph = vox.osc[oscId].morph.get(i);
        auto tableIndex = std::min(tables.numTables - 1, int(float(tables.numTables) * morph));
        auto* t1 = tables.tables.getUnchecked(tableIndex);
        out.data[i] = t1->tableForNote(vox.voice.key[i]).data();
        currIndex[i] = (float)tableIndex;

        int t2idx = i + SIMD_SZ;
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

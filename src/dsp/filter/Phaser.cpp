#include "Phaser.h"

void Phaser::init(SIMDF cutoff, SIMDF resonance, bool reset, SIMDM mask)
{
    Utils::setMasked(cut_targ, cutoff, mask);
    Utils::setMasked(res_targ, resonance, mask);

    auto x = cut_targ.min(srate * kMinNyquistMult) * freqScale;
    Utils::setMasked(g_targ, (x / (x + 1)).min(kMaxRads).tan(), mask);
    Utils::setMasked(k_targ, resonance, mask);

    Utils::setMasked(remove_lows_stage.coeff, (g_targ * kClearRatio).min(0.9f), mask);
    Utils::setMasked(remove_highs_stage.coeff, (g_targ) * (one / kClearRatio), mask);;

    if (reset)
    {
        Utils::setMasked(cut, cut_targ, mask);
        Utils::setMasked(res, res_targ, mask);
        Utils::setMasked(g, g_targ, mask);
        Utils::setMasked(k, k_targ, mask);
        for (int i = 0; i < kMaxStages; ++i) {
            Utils::setMasked(stages[i].coeff, g_targ, mask);
        }
    }
}

void Phaser::processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, SIMDF mask)
{
    switch (type)
    {
        case kPhaserPos: _processBlock<kPhaserPos>(input, start, nsamps, mask); break;
        default: _processBlock<kPhaserNeg>(input, start, nsamps, mask); break;
    }
}

template<Filter::Type type_>
void Phaser::_processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int, int nsamps, SIMDF mask)
{
    // prepare block
    if (!Utils::equal(cut, cut_targ) || !Utils::equal(res, res_targ))
    {
        init(cut_targ, res_targ, false, Utils::floatToMask(mask));
    }

    auto isize = 1.f / nsamps;
    k_step = (k_targ - k) * isize;
    g_step = (g_targ - g) * isize;

    // process
    SIMDF y0;
    SIMDF invert = 1.f;
    if constexpr (type_ == kPhaserNeg) invert = -1.f;
    for (int i = 0; i < nsamps; ++i)
    {
        SIMDF x = input[i];

        SIMDF peak1 = (one - morph * 2.f).sat(0.f, 1.f);
        SIMDF peak5 =  (morph * 2.f - 1.0).sat(0.f, 1.f);
        SIMDF peak3 = -peak1 - peak5 + one;

        SIMDF lows = remove_lows_stage.eval(allpass_output);
        SIMDF highs = remove_highs_stage.eval(lows);
        SIMDF state = k * (lows - highs);

        SIMDF inp = x + invert * state;
        SIMDF output;

        for (int j = 0; j < kPeakStage; ++j)
        {
            output = stages[j].eval(inp);
            inp = inp + output * -2.0f;
        }

        SIMDF peak1out = inp;

        for (int j = kPeakStage; j < 2 * kPeakStage; ++j)
        {
            output = stages[j].eval(inp);
            inp = inp + output * -2.0f;
        }

        SIMDF peak3out = inp;

        for (int j = 2 * kPeakStage; j < 3 * kPeakStage; ++j)
        {
            output = stages[j].eval(inp);
            inp = inp + output * -2.0f;
        }

        SIMDF peak5out = inp;
        SIMDF peak13out = (peak1 * peak1out) + peak3 * peak3out;
        allpass_output = peak13out + peak5 * peak5out;

        out[i] = (x + invert * allpass_output) * 0.5f * mask;

        // interpolation
        g += g_step;
        k += k_step;
        for (int j = 0; j < kMaxStages; ++j) {
            Utils::setMasked(stages[j].coeff, g, {true, true, true, true});
        }
    }

    // finish block
    cut = cut_targ;
    res = res_targ;
    k = k_targ;
    g = g_targ;
}

void Phaser::clear(SIMDF sample, SIMDM mask)
{
    remove_highs_stage.reset(sample, mask);
    remove_lows_stage.reset(sample, mask);
    for (int i = 0; i < kMaxStages; ++i) {
        stages[i].reset(sample, mask);
    }
}

void Phaser::setDrive(SIMDF drive_, SIMDM mask)
{
    Utils::setMasked(morph, drive_, mask);
}
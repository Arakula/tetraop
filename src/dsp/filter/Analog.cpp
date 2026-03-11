#include "Analog.h"

static SIMDF tuneResonance(SIMDF resonance, SIMDF coefficient) {
    return resonance / (coefficient * 0.09f + 0.97f).max(1.f);
}

void Analog::init(SIMDF cutoff, SIMDF resonance, bool reset, SIMDM mask)
{
	Utils::setMasked(cut_targ, cutoff, mask);
    Utils::setMasked(res_targ, resonance, mask);

	auto x = (cut_targ.min(srate * kMinNyquistMult) * freqScale).min(kMaxRads);
    Utils::setMasked(g_targ, (x / (x + 1)).tan(), mask);

	SIMDF k_t = filterMode == BS ? resonance : resonance * 2.15f;
	k_t += drivenorm * resonance * kDriveResonanceBoost;
	Utils::setMasked(k_targ, k_t, mask);

    if (reset)
    {
        Utils::setMasked(cut, cut_targ, mask);
        Utils::setMasked(res, res_targ, mask);
        Utils::setMasked(g, g_targ, mask);
        Utils::setMasked(k, k_targ, mask);
        Utils::setMasked(stage1.coeff, g, mask);
	    Utils::setMasked(stage2.coeff, g, mask);
	    Utils::setMasked(pre_stage1.coeff, g, mask);
	    Utils::setMasked(pre_stage2.coeff, g, mask);
        Utils::setMasked(k_step, 0.f, mask);
        Utils::setMasked(g_step, 0.f, mask);
    }
}

void Analog::processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, int blocksize, SIMDF mask)
{
    switch (type)
    {
        case kAnalog12:
            switch(filterMode)
            {
                case LP: _processBlock<Filter::LP, Filter::k12p>(input, start, nsamps, blocksize, mask); break;
                case HP: _processBlock<Filter::HP, Filter::k12p>(input, start, nsamps, blocksize, mask); break;
                case BP: _processBlock<Filter::BP, Filter::k12p>(input, start, nsamps, blocksize, mask); break;
                case BS: _processBlock<Filter::BS, Filter::k12p>(input, start, nsamps, blocksize, mask); break;
                case PK: _processBlock<Filter::PK, Filter::k12p>(input, start, nsamps, blocksize, mask); break;
            }
            break;
        case kAnalog24:
            switch(filterMode)
            {
                case LP: _processBlock<Filter::LP, Filter::k24p>(input, start, nsamps, blocksize, mask); break;
                case HP: _processBlock<Filter::HP, Filter::k24p>(input, start, nsamps, blocksize, mask); break;
                case BP: _processBlock<Filter::BP, Filter::k24p>(input, start, nsamps, blocksize, mask); break;
                case BS: _processBlock<Filter::BS, Filter::k24p>(input, start, nsamps, blocksize, mask); break;
                case PK: _processBlock<Filter::PK, Filter::k24p>(input, start, nsamps, blocksize, mask); break;
            }
            break;
    }
}

template<Filter::Mode mode, Filter::Slope slope>
void Analog::_processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, int blocksize, SIMDF mask)
{
    // prepare block
    if (start == 0)
    {
        if (!Utils::equal(cut, cut_targ) || !Utils::equal(res, res_targ))
        {
            init(cut_targ, res_targ, false, Utils::floatToMask(mask));
        }
        // steps always recalculated as drive may change k
        auto isize = 1.f / (blocksize - start);
        g_step = (g_targ - g) * isize;
        k_step = (k_targ - k) * isize;
    }

    // process
    for (int i = 0; i < nsamps; ++i)
    {
		SIMDF x = input[i];
		SIMDF output = 0.0;
		SIMDF s1in = 0.0;

		if constexpr (slope == k12p || mode == BS)
		{
            auto tunedRes = tuneResonance(k, g * 2.f);
            SIMDF stg1_mult = g * 2.f - g * g - SIMDF(1.0f);
            auto norm = SIMDF(1.0f) / (tunedRes * ((g * g)-g) + 1.0f);

			SIMDF feedback = stage1.state * stg1_mult + stage2.state * (-g + 1.f);
			s1in = ((drive * x - tunedRes * feedback) * norm).tanh();
			SIMDF s1out = stage1.eval(s1in);
			stage2.eval(s1out);
		}
		else
		{
            auto g2 = g * g;
            auto tunedRes = tuneResonance(k, g * 2.f);
            SIMDF stg1_mult = g * 2.f - g2 - SIMDF(1.0f);
            auto pre_norm = SIMDF(1.f) / (g2 + 1.f);
            auto norm = SIMDF(1.0f) / (tunedRes * (g2 - g) + 1.0f);

			SIMDF feedback = pre_stage1.state * stg1_mult + pre_stage2.state * (-g + 1.f);
			s1in = (x - feedback) * pre_norm;
			SIMDF s1out = pre_stage1.eval(s1in);
			SIMDF s2out = pre_stage2.eval(s1out);
			SIMDF lowout = s2out;
			SIMDF bandout = s1out - lowout;
			SIMDF highout = s1in - s1out + bandout;
			SIMDF preout;
			if constexpr (mode == LP) preout = lowout;
			else if constexpr (mode == BP) preout = bandout;
			else if constexpr (mode == PK) preout = x + bandout;
			else preout = highout;
			feedback = stage1.state * stg1_mult + stage2.state * (-g + 1.f);
			s1in = ((drive * preout - tunedRes * feedback) * norm).tanh();
			s1out = stage1.eval(s1in);
			stage2.eval(s1out);
		}

		SIMDF s2in = stage1.curr;
		SIMDF low = stage2.curr;
		SIMDF band = s2in - low;
		SIMDF high = s1in - s2in - band;

		if constexpr (mode == LP) output = low;
		else if constexpr (mode == BP) output = band;
		else if constexpr (mode == BS) output = x - band;
		else if constexpr (mode == PK) output = x + band;
		else output = high;

		out[i] = output * idrive * mask;

        // interpolate
        g += g_step;
        k += k_step;
        stage1.coeff = g;
        stage2.coeff = g;
        pre_stage1.coeff = g;
        pre_stage2.coeff = g;
    }

    // finish block
    if (start + nsamps >= blocksize)
    {
        cut = cut_targ;
        res = res_targ;
        g = g_targ;
        k = k_targ;
        g_step = 0.f;
        k_step = 0.f;
    }
}

void Analog::clear(SIMDF sample, SIMDM mask)
{
	stage1.reset(sample, mask);
	stage2.reset(sample, mask);
	pre_stage1.reset(sample, mask);
	pre_stage2.reset(sample, mask);
	Utils::setMasked(g, 0.f, mask);
	Utils::setMasked(k, 0.f, mask);
}

void Analog::setDrive(SIMDF drive_, SIMDM mask)
{
    if ((drivenorm - drive_).abs().blend(0.f, mask).hmax() < 1e-6f)
        return; // nothing changed
    Utils::setMasked(drivenorm, drive_, mask);
    auto K = drive_ * MAX_FILTER_DRIVE * DB2LOG;
    auto resScale = res_targ * res_targ * 2.0f + 1.0f;
    Utils::setMasked(drive, K.exp() / resScale, mask);
    Utils::setMasked(idrive, SIMDF(1.0) / (resScale * drive).sqrt(), mask);

    // resonance depends on drive, update resonance
    SIMDF k_t = filterMode == BS ? res_targ : res_targ * 2.15f;
    k_t += drivenorm * res_targ * kDriveResonanceBoost;
    Utils::setMasked(k_targ, k_t, mask);
}
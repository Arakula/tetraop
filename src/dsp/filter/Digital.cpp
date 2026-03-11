#include "Digital.h"

void Digital::init(SIMDF cutoff, SIMDF resonance, bool reset, SIMDM mask)
{
    Utils::setMasked(cut_targ, cutoff, mask);
    Utils::setMasked(res_targ, resonance, mask);

    SIMDF q = res_targ;
    q *= type == kDigital12 ? 0.99f : 0.95f;

    auto x = cut_targ.min(srate * kMinNyquistMult) * freqScale;
    Utils::setMasked(g_targ, x.min(kMaxRads).tan(), mask);
    Utils::setMasked(k_targ, SIMDF(2.f) - q * 2.f, mask);
    if (filterMode == BS)
        Utils::setMasked(k_targ, SIMDF(2.f) - k_targ, mask); // invert notch to k relation

    if (reset)
    {
        Utils::setMasked(cut, cut_targ, mask);
        Utils::setMasked(res, res_targ, mask);
        Utils::setMasked(g, g_targ, mask);
        Utils::setMasked(k, k_targ, mask);
        Utils::setMasked(a1, SIMDF(1.f) / (g * (g + k) + 1), mask);
        Utils::setMasked(a2, g * a1, mask);
        Utils::setMasked(a3, g * a2, mask);
        Utils::setMasked(k_step, 0.f, mask);
        Utils::setMasked(g_step, 0.f, mask);
    }
}



void Digital::processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, int blocksize, SIMDF mask)
{
    switch (type)
    {
        case kDigital12:
            switch(filterMode)
            {
                case LP: _processBlock<Filter::LP, Filter::k12p>(input, start, nsamps, blocksize, mask); break;
                case HP: _processBlock<Filter::HP, Filter::k12p>(input, start, nsamps, blocksize, mask); break;
                case BP: _processBlock<Filter::BP, Filter::k12p>(input, start, nsamps, blocksize, mask); break;
                case BS: _processBlock<Filter::BS, Filter::k12p>(input, start, nsamps, blocksize, mask); break;
                case PK: _processBlock<Filter::PK, Filter::k12p>(input, start, nsamps, blocksize, mask); break;
            }
            break;
        case kDigital24:
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
void Digital::_processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, int blocksize, SIMDF mask)
{
    // prepare block
    if (start == 0)
    {
        if (!Utils::equal(cut, cut_targ) || !Utils::equal(res, res_targ))
        {
            init(cut_targ, res_targ, false, Utils::floatToMask(mask));
            auto isize = 1.f / (blocksize - start);
            g_step = (g_targ - g) * isize;
            k_step = (k_targ - k) * isize;
        }
    }

    const bool hasDrive = drive.hmax() > 1.0001f;

    // process
    for (int i = 0; i < nsamps; ++i)
    {
        SIMDF x = input[i];
        x *= drive;

        // 12p first stage
        auto v3 = x - ic2;
        auto v1 = a1.fmadd(ic1, a2 * v3); // band
        auto v2 = ic2 + a2.fmadd(ic1, a3 * v3); // low

        ic1 = v1.fmadd(2.f, -ic1);
        ic2 = v2.fmadd(2.f, -ic2);

        SIMDF output;
        if constexpr (mode == LP) output = v2;
        else if constexpr (mode == BP) output = v1;
        else if constexpr (mode == HP) output = x - k * v1 - v2;
        else if constexpr (mode == BS) output = x - k * v1;
        else output = x + (SIMDF(2.0f) - k) * v1;

        if constexpr (slope == k12p)
        {
            if (hasDrive) output = hardTanh(output, drive > 1.f);
            out[i] = output * idrive * mask;
        }
        else
        {
            // 24p second stage
            v3 = output - ic4;
            v1 = a1.fmadd(ic3, a2 * v3);
            v2 = ic4 + a2.fmadd(ic3, a3 * v3);
            ic3 = v1.fmadd(2.f, -ic3);
            ic4 = v2.fmadd(2.f, -ic4);

            if constexpr (mode == LP) output = v2;
            else if constexpr (mode == BP) output = v1;
            else if constexpr (mode == HP) output = output - k * v1 - v2;
            else if constexpr (mode == BS) output = output - k * v1;
            else output = output + (SIMDF(2.0f) - k) * v1; // peak

            if (hasDrive) output = hardTanh(output, drive > 1.f);
            out[i] = output * idrive * mask;
        }

        // interpolate
        g += g_step;
        k += k_step;
        a1 = SIMDF(1.f) / (SIMDF(1.f) + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
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

void Digital::clear(SIMDF sample, SIMDM mask)
{
    Utils::setMasked(ic1, sample, mask);
    Utils::setMasked(ic2, sample, mask);
    Utils::setMasked(ic3, sample, mask);
    Utils::setMasked(ic4, sample, mask);
    Utils::setMasked(g, 0.f, mask);
    Utils::setMasked(k, 0.f, mask);
    Utils::setMasked(a1, 0.f, mask);
    Utils::setMasked(a2, 0.f, mask);
    Utils::setMasked(a3, 0.f, mask);
}

void Digital::setDrive(SIMDF drive_, SIMDM mask)
{
    if ((drivenorm - drive_).abs().blend(0.f, mask).hmax() < 1e-6f)
        return; // nothing changed
    Utils::setMasked(drivenorm, drive_, mask);
    auto K = drive_ * MAX_FILTER_DRIVE * DB2LOG;
    Utils::setMasked(drive, K.exp(), mask);
    Utils::setMasked(idrive, (K * -0.6f).exp(), mask);
}
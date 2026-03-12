#include "Ladder.h"

void Ladder::init(SIMDF cutoff, SIMDF resonance, bool reset, SIMDM mask)
{
    Utils::setMasked(cut_targ, cutoff, mask);
    Utils::setMasked(res_targ, resonance, mask);

    cutoff = cutoff.sat(20.f, srate / 2.f);

    Utils::setMasked(f0_targ, (cutoff * -MathConstants<float>::twoPi / srate).exp(), mask);
    Utils::setMasked(k_targ, filterMode == BS ? resonance * 0.5f : resonance.sat(0.1f, 1.f), mask);

    if (reset)
    {
        Utils::setMasked(cut, cut_targ, mask);
        Utils::setMasked(res, res_targ, mask);
        Utils::setMasked(f0, f0_targ, mask);
        Utils::setMasked(k, k_targ, mask);
    }
}

void Ladder::processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, SIMDF mask)
{
    switch (type)
    {
        case kLadder12:
            switch(filterMode)
            {
                case LP: _processBlock<Filter::LP, Filter::k12p>(input, start, nsamps, mask); break;
                case HP: _processBlock<Filter::HP, Filter::k12p>(input, start, nsamps, mask); break;
                case BP: _processBlock<Filter::BP, Filter::k12p>(input, start, nsamps, mask); break;
                case BS: _processBlock<Filter::BS, Filter::k12p>(input, start, nsamps, mask); break;
                case PK: _processBlock<Filter::PK, Filter::k12p>(input, start, nsamps, mask); break;
            }
            break;
        case kLadder24:
            switch(filterMode)
            {
                case LP: _processBlock<Filter::LP, Filter::k24p>(input, start, nsamps, mask); break;
                case HP: _processBlock<Filter::HP, Filter::k24p>(input, start, nsamps, mask); break;
                case BP: _processBlock<Filter::BP, Filter::k24p>(input, start, nsamps, mask); break;
                case BS: _processBlock<Filter::BS, Filter::k24p>(input, start, nsamps, mask); break;
                case PK: _processBlock<Filter::PK, Filter::k24p>(input, start, nsamps, mask); break;
            }
            break;
    }
}

template<Filter::Mode mode, Filter::Slope slope>
void Ladder::_processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int, int nsamps, SIMDF mask)
{
    // prepare block
    if (!Utils::equal(cut, cut_targ) || !Utils::equal(res, res_targ))
    {
        init(cut_targ, res_targ, false, Utils::floatToMask(mask));
    }

    auto isize = 1.f / nsamps;
    f0_step = (f0_targ - f0) * isize;
    k_step = (k_targ - k) * isize;

    // process
    for (int i = 0; i < nsamps; ++i)
    {
        SIMDF x = input[i];

        const SIMDF f = f0;
        const auto g = f * -1.0f + SIMDF(1.0f);
        const auto b0 = g * SIMDF(0.76923076923f);
        const auto b1 = g * SIMDF(0.23076923076f);

        const auto dx = gain * (drive * x).tanh();
        const auto a  = dx + k * -4.0f * (gain2 * (drive2 * state[4]).tanh() - dx * comp);

        const auto b = b1 * state[0] + f * state[1] + b0 * a;
        const auto c = b1 * state[1] + f * state[2] + b0 * b;
        const auto d = b1 * state[2] + f * state[3] + b0 * c;
        const auto e = b1 * state[3] + f * state[4] + b0 * d;

        state[0] = a;
        state[1] = b;
        state[2] = c;
        state[3] = d;
        state[4] = e;

        // For notch or peak mode it does not work perfectly, specially with 24db coefficients
        // Subtracting the band from the signal yields mild results, still better than nothing
        if constexpr (mode == BS)
            out[i] = (x - (b - c)) * mask;

        else if constexpr (mode == PK)
            out[i] = (x + (b - c)) * mask;

        else out[i] = x + mix * ((a * A[0] + b * A[1] + c * A[2] + d * A[3] + e * A[4]) * mask - x);
    }

    // finish block
    cut = cut_targ;
    res = res_targ;
    f0 = f0_targ;
    k = k_targ;
}

void Ladder::setDrive(SIMDF norm, SIMDM mask) {
    if ((drivenorm - norm).abs().blend(0.f, mask).hmax() < 1e-6f)
        return; // nothing changed

    Utils::setMasked(drivenorm, norm, mask);

    auto K = norm * MAX_FILTER_DRIVE * DB2LOG;
    Utils::setMasked(drive, K.exp(), mask);
    Utils::setMasked(drive2, drive*0.01f + 0.99f, mask);
    Utils::setMasked(gain , (drive.log()  * -0.7f).exp(), mask);
    Utils::setMasked(gain2, (drive2.log() * -0.7f).exp(), mask);
};

void Ladder::setMode(Filter::Mode mode_)
{
    filterMode = mode_;
    updateState();
};

void Ladder::updateState()
{
    if (type == kLadder12) {
        if (filterMode == LP) {
            A = {{ 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }};
            comp = 0.5f;
        }
        else if (filterMode == BP) {
            A = {{ 0.0f, 1.0f, -1.0f, 0.0f, 0.0f }};
            comp = 0.5f;
        }
        else {
            A = {{ 1.0f, -2.0f, 1.0f, 0.0f, 0.0f }};
            comp = 0.0f;
        }
    }
    else {
        if (filterMode == LP) {
            A = {{ 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }};
            comp = 0.5f;
        }
        else if (filterMode == BP) {
            A = {{ 0.0f, 0.0f, 1.0f, -2.0f, 1.0f }};
            comp = 0.5f;
        }
        else {
            A = {{ 1.0f, -4.0f, 6.0f, -4.0f,  1.0f }};
            comp = 0.0f;
        }
    }
    if (filterMode == BS || filterMode == PK) {
        comp = 0.0f;
    }

    for (auto& a : A)
        a *= 1.2f; // output gain
}

void Ladder::clear(SIMDF sample, SIMDM mask)
{
    for (int i = 0; i < numStates; ++i)
    {
        Utils::setMasked(state[i], sample, mask);
    }
    Utils::setMasked(f0_targ, 0.f, mask);
    Utils::setMasked(f0, 0.f, mask);
    Utils::setMasked(k_targ, 0.f, mask);
    Utils::setMasked(k, 0.f, mask);
}

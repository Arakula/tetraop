#include "TB303.h"

void TB303::init(SIMDF cutoff, SIMDF resonance, bool reset, SIMDM mask)
{
    Utils::setMasked(cut_targ, cutoff, mask);
    Utils::setMasked(res_targ, resonance, mask);

    SIMDF q =  (one - (resonance * -3.f).exp()) / (one - SIMDF(-3.f).exp());

    // calculate intermediate variables:
    SIMDF wc = cutoff * MathConstants<float>::twoPi * israte;
    SIMDF s,c;
    wc.sincos(s,c);

    SIMDF t  = (SIMDF(0.25f)*(wc-MathConstants<float>::pi)).tan();
    SIMDF r  = q;

    // calculate filter a1-coefficient tuned such the resonance frequency is just right:
    SIMDF a1_fullRes = t / (s-c*t);

    // calculate filter a1-coefficient as if there were no resonance:
    SIMDF x        = (-wc).exp();
    SIMDF a1_noRes = -x;

    // use a weighted sum between the resonance-tuned and no-resonance coefficient:
    SIMDF a1_ = r*a1_fullRes + (one-r)*a1_noRes;

    // calculate the b0-coefficient from the condition that each stage should be a leaky
    // integrator:
    SIMDF b0_ = one+a1_;

    // calculate feedback factor by dividing the resonance parameter by the magnitude at the
    // resonant frequency:
    SIMDF gsq = b0_*b0_ / (one + a1_*a1_ + a1_*c*2.f);
    SIMDF k_          = r / (gsq*gsq);

    Utils::setMasked(a1_targ, a1_, mask);
    Utils::setMasked(b0_targ, b0_, mask);
    Utils::setMasked(k_targ, k_, mask);

    if (reset)
    {
        Utils::setMasked(cut, cut_targ, mask);
        Utils::setMasked(res, res_targ, mask);
        Utils::setMasked(a1, a1_targ, mask);
        Utils::setMasked(b0, b0_targ, mask);
        Utils::setMasked(k, k_targ, mask);
        Utils::setMasked(a1_step, 0.f, mask);
        Utils::setMasked(b0_step, 0.f, mask);
        Utils::setMasked(k_step, 0.f, mask);
    }

    switch(filterMode)
    {
        case LP: c0 =  0.0; c1 =  0.0; c2 =  0.0; c3 =  1.0; c4 =  0.0;  break;
        case HP: c0 =  1.0; c1 = -3.0; c2 =  3.0; c3 = -1.0; c4 =  0.0;  break;
        case BP: c0 =  0.0; c1 =  0.0; c2 =  0.0; c3 =  1.0; c4 = -1.0;  break;
        default: c0 =  0.0; c1 =  0.0; c2 =  0.0; c3 =  1.0; c4 =  0.0;
    }
}



void TB303::processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, SIMDF mask)
{
    switch(filterMode)
    {
        case LP: _processBlock<Filter::LP>(input, start, nsamps, mask); break;
        case HP: _processBlock<Filter::HP>(input, start, nsamps, mask); break;
        case BP: _processBlock<Filter::BP>(input, start, nsamps, mask); break;
        default: _processBlock<Filter::LP>(input, start, nsamps, mask); break;
    }
}

template<Filter::Mode mode>
void TB303::_processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int, int nsamps, SIMDF mask)
{
    // prepare block
    if (!Utils::equal(cut, cut_targ) || !Utils::equal(res, res_targ))
    {
        init(cut_targ, res_targ, false, Utils::floatToMask(mask));
    }

    auto isize = 1.f / nsamps;
    b0_step = (b0_targ - b0) * isize;
    a1_step = (a1_targ - a1) * isize;
    k_step = (k_targ - k) * isize;

    // process
    SIMDF y0;
    for (int i = 0; i < nsamps; ++i)
    {
        SIMDF x = input[i];

        y0 = x*0.125f - highpass.process(k * y4);
        y1 = y0 + a1*(y0-y1);
        y2 = y1 + a1*(y1-y2);
        y3 = y2 + a1*(y2-y3);
        y4 = y3 + a1*(y3-y4);

        SIMDF output = (c0*y0 + c1*y1 + c2*y2 + c3*y3 + c4*y4) * 8.f;
        output = (output * drive).tanh() * idrive;
        out[i] = output * mask * (SIMDF(1.1f) + k/5.f); // some hacky gain compensation

        // interpolate
        b0 += b0_step;
        a1 += a1_step;
        k += k_step;
    }

    // finish block
    cut = cut_targ;
    res = res_targ;
    b0 = b0_targ;
    a1 = a1_targ;
    k = k_targ;
}

void TB303::clear(SIMDF sample, SIMDM mask)
{
    highpass.reset();
    Utils::setMasked(y1, sample, mask);
    Utils::setMasked(y2, sample, mask);
    Utils::setMasked(y3, sample, mask);
    Utils::setMasked(y4, sample, mask);
}

void TB303::setDrive(SIMDF drive_, SIMDM mask)
{
    if ((drivenorm - drive_).abs().blend(0.f, mask).hmax() < 1e-6f)
        return; // nothing changed
    Utils::setMasked(drivenorm, drive_, mask);
    auto K = drive_ * MAX_FILTER_DRIVE * DB2LOG;
    Utils::setMasked(drive, K.exp(), mask);
    Utils::setMasked(idrive, (K * -0.6f).exp(), mask);
}
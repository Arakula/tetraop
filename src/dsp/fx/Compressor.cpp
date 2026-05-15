#include "./Compressor.h"
#include "../../PluginProcessor.h"

Compressor::Compressor(RipplerAudioProcessor& p, int _layer) : FX(p, FX::Compressor, _layer)
{
	makeupParam = audioProcessor.params.getRawParameterValue(prefix + "makeup");
	threshParam = audioProcessor.modulation->getParamHandle(prefix + "thresh");
	ratioParam = audioProcessor.modulation->getParamHandle(prefix + "ratio");
	attackParam = audioProcessor.modulation->getParamHandle(prefix + "att");
	releaseParam = audioProcessor.modulation->getParamHandle(prefix + "rel");
	gainParam = audioProcessor.modulation->getParamHandle(prefix + "gain");
}

Compressor::~Compressor()
{
}

void Compressor::prepare(float _srate)
{
	srate = _srate;
    t = 0.f;
}

void Compressor::processBlock(float* left, float* right, int nsamps, int /*blockoffset*/, bool /*audioRate*/)
{
    auto makeup = (bool)makeupParam->load();
    auto threshdb = audioProcessor.modulation->getValue(threshParam, false, nsamps, srate);
    auto ratio = audioProcessor.modulation->getValue(ratioParam, false, nsamps, srate);
    auto attack = audioProcessor.modulation->getValue(attackParam, false, nsamps, srate);
    auto release = audioProcessor.modulation->getValue(releaseParam, false, nsamps, srate);
    auto volume = audioProcessor.modulation->getValue(gainParam, false, nsamps, srate);

    ratio = 1.f / ratio;
    attack = attack > 0.f ? std::exp(threshdb / (attack * srate / 1000.f) * globals::DB2LOG) : 1e-3f;
    release = release > 0.f ? std::exp(threshdb / (release * srate / 1000.f) * globals::DB2LOG) : 1e-3f;
    volume = std::exp(volume * globals::DB2LOG) / (makeup ? std::exp((threshdb -threshdb * ratio) * globals::DB2LOG) : 1.f);
    auto thresh = std::exp(threshdb * globals::DB2LOG);

    // DC filter
    float b = -exp(-62.83185307f / srate); // exp(10*2*PI / srate) 10Hz cutoff
    float a = 1.0f + b;

    for (int i = 0; i < nsamps; ++i) {
        auto rms = std::max(std::fabs(left[i]), std::fabs(right[i]));
        t = a*rms - b*t; // apply DC filter
        rms = std::max(std::sqrt(t), rms);
        rms = std::max(rms, 1e-10f);

        seekGain = ((rms > thresh)
            ? exp((threshdb + (log(rms)/globals::DB2LOG-threshdb)*ratio) * globals::DB2LOG) / rms
            : 1);

        gain = ((gain > seekGain)
            ? std::max(gain * attack, seekGain)
            : std::min(gain/release, seekGain));

        left[i] = left[i] * gain * volume;
        right[i] = right[i] * gain * volume;
    }

    audioProcessor.compReduction.store(0.5f * audioProcessor.compReduction.load() + 0.5f * gain);
}

#include "./Distortion.h"
#include "../../PluginProcessor.h"

Distortion::Distortion(TetraOPAudioProcessor& p) : FX(p, FX::Distortion) 
{
	modeParam = audioProcessor.params.getRawParameterValue(prefix + "mode");
	driveParam = audioProcessor.modulation->getParamHandle(prefix + "drive");
	gainParam = audioProcessor.modulation->getParamHandle(prefix + "gain");
	mixParam = audioProcessor.modulation->getParamHandle(prefix + "mix");
	filterParam = audioProcessor.modulation->getParamHandle(prefix + "filter");
	colorParam = audioProcessor.modulation->getParamHandle(prefix + "color");

	audioProcessor.params.addParameterListener(prefix + "on", this);
	audioProcessor.params.addParameterListener(prefix + "bypass", this);
}

Distortion::~Distortion() 
{
	audioProcessor.params.removeParameterListener(prefix + "on", this);
	audioProcessor.params.removeParameterListener(prefix + "bypass", this);
}

void Distortion::parameterChanged(const juce::String& paramId, float value)
{
	(void)paramId;
	(void)value;
	queueReset = true;
}

void Distortion::prepare(float _srate)
{
	srate = _srate * 2.f; // hardcoded 2x oversampling
	filterL.mode = SVF::Off;
	filterR.mode = SVF::Off;
	colorL.mode = SVF::Off;
	colorR.mode = SVF::Off;

	drive = audioProcessor.modulation->getValue(driveParam);
	gain = audioProcessor.modulation->getValue(gainParam);
	mix = audioProcessor.modulation->getValue(mixParam);
	dryLeft.resize(audioProcessor.samplesPerBlock * 2);
	dryRight.resize(audioProcessor.samplesPerBlock * 2);
}

void Distortion::processBlock(float* left, float* right, int nsamps, int /*blockoffset*/, bool /*audioRate*/)
{
	if (queueReset) {
		filterL.clear(0.f);
		filterR.clear(0.f);
		colorL.clear(0.f);
		colorR.clear(0.f);
		audioProcessor.distoversampler->reset();
		drive = audioProcessor.modulation->getValue(driveParam, false, nsamps);
		gain = audioProcessor.modulation->getValue(gainParam, false, nsamps);
		mix = audioProcessor.modulation->getValue(mixParam, false, nsamps);
		drive = std::exp(drive * globals::DB2LOG);
		gain = std::exp(gain * globals::DB2LOG);
		queueReset = false;
	}

	std::function<float(float)> dist;
	int mode = (int)modeParam->load();
	float drive_targ = audioProcessor.modulation->getValue(driveParam, false, nsamps);
	float filter = audioProcessor.modulation->getValue(filterParam, false, nsamps);
	float color = audioProcessor.modulation->getValue(colorParam, false, nsamps);
	float gain_targ = audioProcessor.modulation->getValue(gainParam, false, nsamps);
	float mix_targ = audioProcessor.modulation->getValue(mixParam, false, nsamps);

	drive_targ = std::exp(drive_targ * globals::DB2LOG);
	gain_targ = std::exp(gain_targ * globals::DB2LOG);

	auto drive_step = (drive_targ - drive) / nsamps;
	auto gain_step = (gain_targ - gain) / nsamps;
	auto mix_step = (mix_targ - mix) / nsamps;

	if (mode == 0) dist = [](float x) { return std::tanh(x); };
	else if (mode == 1) dist = [](float x) { return 2.f / (1.f + std::exp(-x)) - 1.f; };
	else dist = [](float x) { return std::tanh(x + 0.3f * (x * x)); };

	auto ffreq = 20.0f * std::pow(1000.f, filter < 0.f ? 1 + filter : filter); // map -1,1 to 20...20000
	if (filter == 0.f) {
		filterL.mode = SVF::Off;
		filterR.mode = SVF::Off;
	}
	else {
		auto fmode = filter > 0.f ? SVF::HP : SVF::LP;
		if (fmode != filterL.mode) {
			filterL.clear(0.f);
			filterR.clear(0.f);
			if (fmode == SVF::LP) {
				filterL.lp(srate, ffreq, 0.707f);
				filterR.lp(srate, ffreq, 0.707f);
			}
			else {
				filterL.hp(srate, ffreq, 0.707f);
				filterR.hp(srate, ffreq, 0.707f);
			}
		}
	}

	auto cfreq = 20.f * std::pow(1000.f, color < 0.f ? 1 + color : color); // map -1,1 to 20...20000
	if (color == 0.f) {
		colorL.mode = SVF::Off;
		colorR.mode = SVF::Off;
	}
	else {
		auto cmode = color > 0.f ? SVF::HP : SVF::LP;
		if (cmode != colorL.mode) {
			colorL.clear(0.f);
			colorR.clear(0.f);
			if (cmode == SVF::LP) {
				colorL.lp(srate, cfreq, 0.707f);
				colorR.lp(srate, cfreq, 0.707f);
			}
			else {
				colorL.hp(srate, cfreq, 0.707f);
				colorR.hp(srate, cfreq, 0.707f);
			}
		}
	}
	jassert(nsamps <= (int)dryLeft.size());
	if (nsamps > (int)dryLeft.size()) {
		dryLeft.resize(nsamps);
		dryRight.resize(nsamps);
	}

	std::copy(left, left + nsamps, dryLeft.data());
	std::copy(right, right + nsamps, dryRight.data());

	filterL.processBlock(left, nsamps, 0, nsamps, ffreq, 0.707f);
	filterR.processBlock(right, nsamps, 0, nsamps, ffreq, 0.707f);

	float makeup = 1.f / (drive >= 1.f ? dist(drive) : 1.f);
	for (int i = 0; i < nsamps; ++i) {
		left[i] = dist(left[i] * drive) * makeup * gain;
		right[i] = dist(right[i] * drive) * makeup * gain;
		drive += drive_step;
		gain += gain_step;
	}

	colorL.processBlock(left, nsamps, 0, nsamps, cfreq, 0.707f);
	colorR.processBlock(right, nsamps, 0, nsamps, cfreq, 0.707f);

	for (int i = 0; i < nsamps; ++i) {
		auto drymix = Utils::cosHalfPi()(mix);
		auto wetmix = Utils::sinHalfPi()(mix);
		left[i] = drymix * dryLeft[i] + wetmix * left[i];
		right[i] = drymix * dryRight[i] + wetmix * right[i];
		mix += mix_step;
	}
	
	drive = drive_targ;
	gain = gain_targ;
	mix = mix_targ;
}

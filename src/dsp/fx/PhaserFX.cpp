
#include "./PhaserFX.h"
#include "../../PluginProcessor.h"

PhaserFX::PhaserFX(TetraOPAudioProcessor& p)
	: FX(p, FX::Phaser)
{
	rateSyncParam = audioProcessor.params.getRawParameterValue(prefix + "rate_sync");
	syncParam = audioProcessor.params.getRawParameterValue(prefix + "sync");
    centerParam = audioProcessor.modulation->getParamHandle(prefix + "center");
	depthParam  = audioProcessor.modulation->getParamHandle(prefix + "depth");
	rateParam   = audioProcessor.modulation->getParamHandle(prefix + "rate");
	resParam    = audioProcessor.modulation->getParamHandle(prefix + "res");
	morphParam  = audioProcessor.modulation->getParamHandle(prefix + "morph");
	stereoParam = audioProcessor.modulation->getParamHandle(prefix + "stereo");
	mixParam    = audioProcessor.modulation->getParamHandle(prefix + "mix");
}

PhaserFX::~PhaserFX()
{
}

void PhaserFX::prepare ( float _srate )
{
    srate = _srate;
}

void PhaserFX::clear()
{
	lphaser.reset(0.f);
	rphaser.reset(0.f);
}

float PhaserFX::getLfoRate(int sync)
{
	if (sync == 0)
		return audioProcessor.modulation->getValue(rateParam, false, 0, srate);

	auto rateSync = rateSyncParam->load();

	auto secondsPerBeat = audioProcessor.secondsPerBeat;
	if (secondsPerBeat == 0.f)
		secondsPerBeat = 0.25f;

	float qn = 1.f;
	if (rateSync == 0) qn = 1.f / 8.f; // 1/32
	if (rateSync == 1) qn = 1.f / 4.f; // 1/16
	if (rateSync == 2) qn = 1.f / 2.f; // 1/8
	if (rateSync == 3) qn = 1.f / 1.f; // 1/4
	if (rateSync == 4) qn = 1.f * 2.f; // 1/2
	if (rateSync == 5) qn = 1.f * 4.f; // 1/1
	if (rateSync == 6) qn = 2.f * 4.f; // 2/1
	if (rateSync == 7) qn = 4.f * 4.f; // 4/1
	if (rateSync == 8) qn = 8.f * 4.f; // 8/1
	if (rateSync == 9) qn = 16.f * 4.f; // 16/1
	if (rateSync == 10) qn = 32.f * 4.f; // 32/1
	if (sync == 2) qn *= 2 / 3.f; // tripplet
	if (sync == 3) qn *= 1.5f; // dotted

	float seconds = (float)(qn * secondsPerBeat);
	return 1.f / seconds;
}

void PhaserFX::processBlock ( float* left, float* right, int nsamps, int /*blockoffset*/, bool /*audioRate*/)
{
	mix.set(audioProcessor.modulation->getValue(mixParam, false, nsamps, srate));
	float morph = audioProcessor.modulation->getValue(morphParam, false, 0, srate);
	float res = audioProcessor.modulation->getValue(resParam, false, 0, srate);
	lfo_center.set(audioProcessor.modulation->getValue(centerParam, false, 0, srate));
	float depth = audioProcessor.modulation->getValue(depthParam, false, 0, srate) * 4;
	float stereo = audioProcessor.modulation->getValue(stereoParam, false, 0, srate) * 0.25f;
	int sync = (int)syncParam->load();
	float rate = getLfoRate(sync);
	lphaser.setMorph(morph);
	rphaser.setMorph(morph);
	lfoL.setRate(rate);
	lfoR.setRate(rate);

	if (audioProcessor.playing)
	{
		lfoL.syncToSongTime(audioProcessor.timeInSeconds);
		lfoR.syncToSongTime(audioProcessor.timeInSeconds);
	}

	for (int i = 0; i < nsamps; ++i)
	{
		float leftLFO  = lfoL.tick(0.0f);
		float rightLFO = lfoR.tick(stereo);
		float freqL = std::clamp((float)(lfo_center.get() * std::pow(2.f, depth * leftLFO)), 1.f, srate * 0.48f);
		float freqR = std::clamp((float)(lfo_center.get() * std::pow(2.f, depth * rightLFO)), 1.f, srate * 0.48f);
		lphaser.init(srate, freqL, res);
		rphaser.init(srate, freqR, res);

		float _mix = mix.get();
		left[i] = _mix * lphaser.eval(left[i]) + (1-_mix) * left[i];
		right[i] = _mix * rphaser.eval(right[i]) + (1-_mix) * right[i];

		mix.tick();
		lfo_center.tick();
	}
}

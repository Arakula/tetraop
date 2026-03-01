#include "OSC.h"
#include "../PluginProcessor.h"

OSC::OSC(int _id, int _voiceId, TetraOPAudioProcessor& p)
	: id(_id)
	, voiceId(_voiceId)
	, prefix(_id == 0 ? "a_" : _id == 1 ? "b_" : _id == 2 ? "c_" : "d_")
	, audioProcessor(p)
{
}

void OSC::prepareBlock(int startSample, int numSamples)
{
	auto& mod = audioProcessor.modulation;
	isOn = mod->getValue(prefix + "level");
	level = mod->getPolyValue(prefix + "level", voiceId, numSamples) * isOn;
	if (level <= 0.f) return;
	pan = mod->getPolyValue(prefix + "pan", voiceId, numSamples);

	auto unison_v = (int)mod->getValue(prefix + "unison_voices", true);
	auto unison_mod = (int)mod->getValue(prefix + "unison_mode", true);
	auto unison_det = mod->getPolyValue(prefix + "unison_detune", voiceId, numSamples);
	auto unison_st = mod->getPolyValue(prefix + "unison_stereo", voiceId, numSamples);
	auto unison_sprd = mod->getPolyValue(prefix + "unison_spread", voiceId, numSamples);
	auto unison_bld = mod->getPolyValue(prefix + "unison_blend", voiceId, numSamples);

	if (unison_v == 1)
	{
		unison_voices = unison_v;
	}
	else if (unison_v != unison_voices || unison_mod != unison_mode
		|| unison_det != unison_detune || unison_st != unison_stereo
		|| unison_sprd != unison_spread || unison_bld != unison_blend
	)
	{
		unison_voices = unison_v;
		unison_mode = unison_mod;
		unison_detune = unison_det;
		unison_stereo = unison_st;
		unison_spread = unison_sprd;
		unison_blend = unison_bld;
		recalcUnison();
	}
}

void OSC::recalcUnison()
{
	std::fill(unison_mask.begin(), unison_mask.end(), 0.f);
	for (int i = 0; i < unison_voices; ++i)
		unison_mask[i] = 1.f;

	unison_ratio = Unison::generateDetuneRatios(unison_voices, unison_detune);
	auto pan = Unison::generateVoicesPan(unison_voices, unison_stereo);
	for (int i = 0; i < unison_voices; i += SIMD_SZ)
	{
		SIMDF p = mipp::load(&pan[i]);
		auto lr = Utils::panToGainCheap(p);
		lr[0].store(&unison_gain_l[i]);
		lr[1].store(&unison_gain_r[i]);
	}
}
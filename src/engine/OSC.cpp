#include "OSC.h"
#include "../PluginProcessor.h"

OSC::OSC(int _id, int _voiceId, TetraOPAudioProcessor& p)
	: id(_id)
	, voiceId(_voiceId)
	, prefix(_id == 0 ? "a_" : _id == 1 ? "b_" : _id == 2 ? "c_" : "d_")
	, audioProcessor(p)
{
}

void OSC::trigger(int note, float srate)
{
	auto& mod = audioProcessor.modulation;
	phase = 0.f;
	freq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
	phase_inc = freq / srate;
	out = 0.f;
	auto phase_start = mod->getValue(prefix + "phase_start");
	auto phase_rand = mod->getValue(prefix + "phase_rand");
	unison_phase = Unison::generatePhases(phase_start, phase_rand);
	auto ntables = audioProcessor.wavetables[id].numTables;
	auto idx = std::min(ntables - 1, int(float(ntables) * mod->getPolyValue(prefix + "morph", voiceId)));
	morph = morph_targ = idx / (float)ntables + 1e-4; // 1e-4 so that morph lerps to floor(morph) == tableIndex
	isOn = mod->getValue(prefix + "on");
	level = level_targ = mod->getPolyValue(prefix + "level", voiceId, 0) * isOn;
}

void OSC::prepareBlock(int startSample, int numSamples)
{
	(void)startSample;
	auto& mod = audioProcessor.modulation;
	isOn = mod->getValue(prefix + "on");
	level_targ = mod->getPolyValue(prefix + "level", voiceId, numSamples) * isOn;
	if (level < 1e-4 && level_targ < 1e-4) return;

	auto pan_targ = mod->getPolyValue(prefix + "pan", voiceId, numSamples);
	if (pan != pan_targ)
	{
		pan = pan_targ;
		gain_l = std::cos(MathConstants<float>::halfPi * pan);
		gain_r = std::sin(MathConstants<float>::halfPi * pan);
	}

	auto ntables = audioProcessor.wavetables[id].numTables;
	auto idx = std::min(ntables - 1, int(float(ntables) * mod->getPolyValue(prefix + "morph", voiceId)));
	morph_targ = idx / (float)ntables + 1e-4; // 1e-4 so that morph lerps to floor(morph) == tableIndex

	auto unison_v = (int)mod->getValue(prefix + "unison_voices", true);
	auto unison_mod = (int)mod->getValue(prefix + "unison_mode", true);
	auto unison_det = mod->getPolyValue(prefix + "unison_detune", voiceId, numSamples);
	auto unison_st = mod->getPolyValue(prefix + "unison_stereo", voiceId, numSamples);
	auto unison_sprd = mod->getPolyValue(prefix + "unison_spread", voiceId, numSamples);
	auto unison_bld = mod->getPolyValue(prefix + "unison_blend", voiceId, numSamples);
	feedback = mod->getPolyValue(prefix + "feedback", voiceId, numSamples);

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

	unison_ratio = Unison::generateDetuneRatios(unison_voices, unison_detune, unison_spread);
	auto panarr = Unison::generateVoicesPan(unison_voices, unison_stereo);
	auto gainarr = Unison::generateVoicesGain(unison_voices, unison_blend);
	for (int i = 0; i < unison_voices; i += SIMD_SZ)
	{
		SIMDF p = mipp::load(&panarr[i]);
		auto lr = Utils::panToGainCheap(p);
		SIMDF gain = mipp::load(&gainarr[i]);
		(lr[0] * gain).store(&unison_gain_l[i]);
		(lr[1] * gain).store(&unison_gain_r[i]);
	}
}
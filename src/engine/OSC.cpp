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
	freq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
	phase_inc = freq / srate;
	out = 0.f;
	auto phase_rand = mod->getPolyValue(prefix + "phase_rand", voiceId, 0);
	phase_offset = mod->getPolyValue(prefix + "phase_offset", voiceId, 0);
	phase = (rand() / (float)RAND_MAX) * phase_rand;
	unison_phase = Unison::generatePhases(phase_rand);
	auto ntables = audioProcessor.wavetables[id].numTables;
	auto idx = std::min(ntables - 1, int(float(ntables) * mod->getPolyValue(prefix + "morph", voiceId)));
	morph = morph_targ = idx / (float)ntables + 1e-4f; // 1e-4 so that morph lerps to floor(morph) == tableIndex
	isOn = mod->getValue(prefix + "on");
	level = level_targ = mod->getPolyValue(prefix + "level", voiceId, 0) * isOn;

	auto pitch_cents = mod->getPolyValue(prefix + "pitch_cents", voiceId, 0);
	auto pitch_semis = mod->getPolyValue(prefix + "pitch_semis", voiceId, 0);
	auto pitch_oct = mod->getPolyValue(prefix + "pitch_oct", voiceId, 0);
	auto total_cents = pitch_cents + (pitch_semis * 100.0f) + (pitch_oct * 1200.0f);
	pitch_ratio = pitch_ratio_targ = Utils::centsToRatio(total_cents);
}

void OSC::prepareBlock(int startSample, int numSamples)
{
	(void)startSample;
	int blkoffset = numSamples; // TODO - should take into account subblock offset?
	auto& mod = audioProcessor.modulation;
	isOn = mod->getValue(prefix + "on");
	level_targ = mod->getPolyValue(prefix + "level", voiceId, blkoffset) * isOn;
	if (level < 1e-4f && level_targ < 1e-4f) return;

	phase_offset = mod->getPolyValue(prefix + "phase_offset", voiceId, blkoffset);
	auto pan_targ = mod->getPolyValue(prefix + "pan", voiceId, blkoffset);
	if (pan != pan_targ)
	{
		pan = pan_targ;
		gain_l = std::cos(MathConstants<float>::halfPi * pan);
		gain_r = std::sin(MathConstants<float>::halfPi * pan);
	}

	auto ntables = audioProcessor.wavetables[id].numTables;
	auto idx = std::min(ntables - 1, int(float(ntables) * mod->getPolyValue(prefix + "morph", voiceId, blkoffset)));
	morph_targ = idx / (float)ntables + 1e-4f; // 1e-4 so that morph lerps to floor(morph) == tableIndex

	auto unison_v = (int)mod->getValue(prefix + "unison_voices", true);
	auto unison_mod = (int)mod->getValue(prefix + "unison_mode", true);
	auto unison_det = mod->getPolyValue(prefix + "unison_detune", voiceId, blkoffset);
	auto unison_st = mod->getPolyValue(prefix + "unison_stereo", voiceId, blkoffset);
	auto unison_sprd = mod->getPolyValue(prefix + "unison_spread", voiceId, blkoffset);
	auto unison_bld = mod->getPolyValue(prefix + "unison_blend", voiceId, blkoffset);
	feedback = mod->getPolyValue(prefix + "feedback", voiceId, blkoffset);

	auto pitch_cents = mod->getPolyValue(prefix + "pitch_cents", voiceId, blkoffset);
	auto pitch_semis = mod->getPolyValue(prefix + "pitch_semis", voiceId, blkoffset);
	auto pitch_oct = mod->getPolyValue(prefix + "pitch_oct", voiceId, blkoffset);
	auto total_cents = pitch_cents + (pitch_semis * 100.0f) + (pitch_oct * 1200.0f);
	pitch_ratio_targ = Utils::centsToRatio(total_cents);

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

void OSC::finishBlock(int)
{
	level = level_targ;
	pitch_ratio = pitch_ratio_targ;
	morph = morph_targ;
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
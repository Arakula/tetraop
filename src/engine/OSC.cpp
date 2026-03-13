#include "OSC.h"
#include "../PluginProcessor.h"

OSC::OSC(int _id, int _voiceId, TetraOPAudioProcessor& p)
	: id(_id)
	, voiceId(_voiceId)
	, prefix(_id == 0 ? "a_" : _id == 1 ? "b_" : _id == 2 ? "c_" : "d_")
	, batch(_voiceId / SIMDSZ)
	, lane(_voiceId % SIMDSZ)
	, audioProcessor(p)
{
}

void OSC::trigger(int note, float srate)
{
	auto& vox = audioProcessor.synth->vox[batch];
	auto& osc = vox.osc[id];
	bool msk[4] = {false, false, false, false};
	msk[lane] = true;
	SIMDM mask = SIMDM(msk);

	auto& mod = audioProcessor.modulation;
	Utils::setMasked(osc.freq, 440.0f * std::pow(2.0f, (note - 69) / 12.0f), mask);
	Utils::setMasked(osc.phase_inc, osc.freq.get(lane) / srate, mask);
	Utils::setMasked(osc.out, 0.f, mask);
	Utils::setMasked(osc.phase_offset, mod->getPolyValue(prefix + "phase_offset", voiceId, 0), mask);
	auto phase_rand = mod->getPolyValue(prefix + "phase_rand", voiceId, 0);
	Utils::setMasked(osc.phase, (rand() / (float)RAND_MAX) * phase_rand, mask);
	auto unison_phase = Unison::generatePhases(phase_rand);
	for (int i = 0; i < MAX_UNISON / SIMDSZ; ++i)
		osc.unison[lane].phase[i].load(&unison_phase[i * SIMDSZ]);

	bool morph_snap = (bool)audioProcessor.params.getRawParameterValue(prefix + "morph_snap")->load();
	float morph = mod->getPolyValue(prefix + "morph", voiceId);
	if (morph_snap)
	{
		auto ntables = audioProcessor.wavetables[id].numTables;
		auto idx = std::min(ntables - 1, int(float(ntables) * morph));
		morph = idx / (float)ntables + 1e-4f; // 1e-4 so that morph lerps to floor(morph) == tableIndex
	}
	Utils::setMasked(osc.morph, morph, mask);
	Utils::setMasked(osc.morph_targ, morph, mask);
	osc.morph_snap = morph_snap;

	bool isOn = mod->getValue(prefix + "on");
	auto level = mod->getPolyValue(prefix + "level", voiceId, 0) * isOn;
	osc.isOn = isOn;
	Utils::setMasked(osc.level, level, mask);
	Utils::setMasked(osc.level_targ, level, mask);

	auto pitch_cents = mod->getPolyValue(prefix + "pitch_cents", voiceId, 0);
	auto pitch_semis = mod->getPolyValue(prefix + "pitch_semis", voiceId, 0);
	auto pitch_oct = mod->getPolyValue(prefix + "pitch_oct", voiceId, 0);
	auto total_cents = pitch_cents + (pitch_semis * 100.0f) + (pitch_oct * 1200.0f);
	auto pitch_ratio = Utils::centsToRatio(total_cents);
	Utils::setMasked(osc.pitch_ratio, pitch_ratio, mask);
	Utils::setMasked(osc.pitch_ratio_targ, pitch_ratio, mask);

	auto seed = phase_rand > 0 ? rand() + 1000 : id * 1000 + 1000;
	osc.whiteNoiseGen[lane].reseed(seed);
	osc.pinkNoiseGen[lane].reseed(seed);
}

void OSC::startBlock(int startSample, int numSamples)
{
	auto& vox = audioProcessor.synth->vox[batch];
	auto& osc = vox.osc[id];
	auto& unison = osc.unison[lane];
	bool msk[4] = { false, false, false, false };
	msk[lane] = true;
	SIMDM mask = SIMDM(msk);

	int blkoffset = startSample - audioProcessor.currBlockPos + numSamples;
	auto& mod = audioProcessor.modulation;

	auto isOn = mod->getValue(prefix + "on");
	osc.isOn = isOn;
	Utils::setMasked(osc.level_targ, mod->getPolyValue(prefix + "level", voiceId, blkoffset) * isOn, mask);
	if (osc.level.get(lane) < 1e-4f && osc.level_targ.get(lane) < 1e-4f) 
		return; // oscillator is off

	Utils::setMasked(osc.phase_offset, mod->getPolyValue(prefix + "phase_offset", voiceId, blkoffset), mask);
	auto pan_targ = mod->getPolyValue(prefix + "pan", voiceId, blkoffset);
	if (pan != pan_targ)
	{
		pan = pan_targ;
		Utils::setMasked(osc.gain_l, std::cos(MathConstants<float>::halfPi * pan), mask);
		Utils::setMasked(osc.gain_r, std::sin(MathConstants<float>::halfPi * pan), mask);
	}

	bool morph_snap = (bool)audioProcessor.params.getRawParameterValue(prefix + "morph_snap")->load();
	float morph = mod->getPolyValue(prefix + "morph", voiceId, blkoffset);
	if (morph_snap)
	{
		auto ntables = audioProcessor.wavetables[id].numTables;
		auto idx = std::min(ntables - 1, int(float(ntables) * morph));
		Utils::setMasked(osc.morph_targ, idx / (float)ntables + 1e-4f, mask); // 1e-4 so that morph lerps to floor(morph) == tableIndex
	}
	else
	{
		Utils::setMasked(osc.morph_targ, morph, mask);
	}
	osc.morph_snap = morph_snap;

	Utils::setMasked(osc.feedback, mod->getPolyValue(prefix + "feedback", voiceId, blkoffset), mask);
	auto pitch_cents = mod->getPolyValue(prefix + "pitch_cents", voiceId, blkoffset);
	auto pitch_semis = mod->getPolyValue(prefix + "pitch_semis", voiceId, blkoffset);
	auto pitch_oct = mod->getPolyValue(prefix + "pitch_oct", voiceId, blkoffset);
	auto total_cents = pitch_cents + (pitch_semis * 100.0f) + (pitch_oct * 1200.0f);

	Utils::setMasked(osc.pitch_ratio_targ, Utils::centsToRatio(total_cents), mask);
	Utils::setMasked(osc.dist_amt, mod->getPolyValue(prefix + "phase_dist_amt", voiceId, blkoffset), mask);

	auto unison_v = (int)mod->getValue(prefix + "unison_voices", true);
	auto unison_mod = (int)mod->getValue(prefix + "unison_mode", true);
	auto unison_det = mod->getPolyValue(prefix + "unison_detune", voiceId, blkoffset);
	auto unison_st = mod->getPolyValue(prefix + "unison_stereo", voiceId, blkoffset);
	auto unison_sprd = mod->getPolyValue(prefix + "unison_spread", voiceId, blkoffset);
	auto unison_bld = mod->getPolyValue(prefix + "unison_blend", voiceId, blkoffset);

	
	if (unison_v > 1 && (unison_v != unison.voices || unison_mod != unison_mode
		|| unison_det != unison_detune || unison_st != unison_stereo
		|| unison_sprd != unison_spread || unison_bld != unison_blend
	))
	{
		unison.voices = unison_v;
		unison_mode = unison_mod;
		unison_detune = unison_det;
		unison_stereo = unison_st;
		unison_spread = unison_sprd;
		unison_blend = unison_bld;
		recalcUnison(unison);
	}
	unison.voices = unison_v;

	bool isFMOutput = audioProcessor.synth->fm->isOut[id].hmax() > 0.f;
	if (!isFMOutput || (osc.level.get(lane) <= 1e-5f && osc.level_targ.get(lane) < 1e-5f))
		unison.voices = 1; // TODO remove this?

	if (unison.voices > 1)
		for (int i = 0; i < 4; ++i)
			unison.inc[i] = osc.phase_inc * unison.ratio[i];
}

void OSC::recalcUnison(SIMDUnison& unison) const
{
	alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> unison_mask;
	for (int i = 0; i < unison.voices; ++i)
		unison_mask[i] = 1.f;

	alignas(sizeof(SIMDF)) auto unison_ratio = Unison::generateDetuneRatios(unison.voices, unison_detune, unison_spread);
	for (int i = 0; i < 4; ++i)
	{
		unison.mask[i].load(&unison_mask[i * 4]);
		unison.ratio[i].load(&unison_ratio[i * 4]);
	}

	auto panarr = Unison::generateVoicesPan(unison.voices, unison_stereo);
	auto gainarr = Unison::generateVoicesGain(unison.voices, unison_blend);
	for (int i = 0; i < unison.voices; i += SIMDSZ)
	{
		SIMDF p = mipp::load(&panarr[i]);
		auto lr = Utils::panToGainCheap(p);
		SIMDF gain = mipp::load(&gainarr[i]);
		unison.gain_l[i / SIMDSZ] = lr[0] * gain;
		unison.gain_r[i / SIMDSZ] = lr[1] * gain;
	}
}
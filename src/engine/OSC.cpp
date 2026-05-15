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

void OSC::trigger(int note, float _srate)
{
	triggered = true;
	srate = _srate;
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
		auto ntables = audioProcessor.tablesMgr->wavetables[id].numTables;
		if (ntables == 0)
		{
			morph = 0.f;
		}
		else
		{
			auto idx = std::min(ntables - 1, int(float(ntables) * morph));
			morph = idx / (float)ntables + 1e-4f; // 1e-4 so that morph lerps to floor(morph) == tableIndex
		}
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

void OSC::retrigger(int note, float _srate)
{
	srate = _srate;
	auto& vox = audioProcessor.synth->vox[batch];
	auto& osc = vox.osc[id];
	bool msk[4] = { false, false, false, false };
	msk[lane] = true;
	SIMDM mask = SIMDM(msk);

	Utils::setMasked(osc.freq, 440.0f * std::pow(2.0f, (note - 69) / 12.0f), mask);
	Utils::setMasked(osc.phase_inc, osc.freq.get(lane) / srate, mask);
}

void OSC::startBlock(int startSample, int numSamples)
{
	float isamps = 1.0f / numSamples;
	auto voice = (Voice*)audioProcessor.synth->getVoice(voiceId);
	auto& vox = audioProcessor.synth->vox[batch];
	auto& osc = vox.osc[id];
	auto& unison = osc.unison[lane];
	bool msk[4] = { false, false, false, false };
	msk[lane] = true;
	SIMDM mask = SIMDM(msk);

	// recalc frequency during glide
	if (voice->glide)
	{
		Utils::setMasked(osc.freq, 440.0f * std::pow(2.0f, (voice->glide_curr * 127.f - 69) / 12.0f), mask);
		Utils::setMasked(osc.phase_inc, osc.freq.get(lane) / srate, mask);
	}

	int blkoffset = startSample - audioProcessor.currBlockPos + numSamples;
	auto& mod = audioProcessor.modulation;

	auto isOn = mod->getValue(prefix + "on");
	osc.isOn = isOn;
	Utils::setMasked(osc.level_targ, mod->getPolyValue(prefix + "level", voiceId, blkoffset) * isOn, mask);
	if (osc.level.get(lane) < 1e-4f && osc.level_targ.get(lane) < 1e-4f) 
		return; // oscillator is off

	// phase_offset
	float phase_offset_targ = mod->getPolyValue(prefix + "phase_offset", voiceId, blkoffset);
	float phase_offset_step = (phase_offset_targ - osc.phase_offset.get(lane)) * isamps;
	Utils::setMasked(osc.phase_offset_step, phase_offset_step, mask);
	if (triggered)
	{
		Utils::setMasked(osc.phase_offset, phase_offset_targ, mask);
		Utils::setMasked(osc.phase_offset_step, 0.f, mask);
	}
	
	// pan gain
	auto pan_targ = mod->getPolyValue(prefix + "pan", voiceId, blkoffset);
	if (pan != pan_targ)
	{
		pan = pan_targ;
		float targ_l = std::cos(MathConstants<float>::halfPi * pan);
		float targ_r = std::sin(MathConstants<float>::halfPi * pan);
		float gain_l_step = (targ_l - osc.gain_l.get(lane)) * isamps;
		float gain_r_step = (targ_r - osc.gain_r.get(lane)) * isamps;
		Utils::setMasked(osc.gain_l_step, gain_l_step, mask);
		Utils::setMasked(osc.gain_r_step, gain_r_step, mask);
		if (triggered) 
		{
			Utils::setMasked(osc.gain_l, targ_l, mask);
			Utils::setMasked(osc.gain_r, targ_r, mask);
			Utils::setMasked(osc.gain_l_step, 0.f, mask);
			Utils::setMasked(osc.gain_r_step, 0.f, mask);
		}
	}
	else
	{
		// FIX - pan gain recalc is only triggered when pan changes
		// this ensures pan gain doesnt drift
		Utils::setMasked(osc.gain_l_step, 0.f, mask);
		Utils::setMasked(osc.gain_r_step, 0.f, mask);
	}

	bool morph_snap = (bool)audioProcessor.params.getRawParameterValue(prefix + "morph_snap")->load();
	float morph = mod->getPolyValue(prefix + "morph", voiceId, blkoffset);
	if (morph_snap)
	{
		auto ntables = audioProcessor.tablesMgr->wavetables[id].numTables;
		if (ntables == 0)
		{
			Utils::setMasked(osc.morph_targ, 0.f, mask);
		} 
		else
		{
			auto idx = std::min(ntables - 1, int(float(ntables) * morph));
			Utils::setMasked(osc.morph_targ, idx / (float)ntables + 1e-4f, mask); // 1e-4 so that morph lerps to floor(morph) == tableIndex
		}
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
	auto pitch_global = audioProcessor.modulation->globalPitchSemis;
	auto pitch_bend = (float)audioProcessor.synth->getVoice(voiceId)->getCurrentlyPlayingNote().totalPitchbendInSemitones;
	auto total_cents = pitch_cents + (pitch_semis * 100.0f) + (pitch_global * 100.f) + (pitch_oct * 1200.0f) + (pitch_bend * 100.f);

	Utils::setMasked(osc.pitch_ratio_targ, Utils::centsToRatio(total_cents), mask);

	float dist_amt_targ = mod->getPolyValue(prefix + "phase_dist_amt", voiceId, blkoffset);
	Utils::setMasked(osc.dist_amt_targ, dist_amt_targ, mask);
	if (triggered)
	{
		Utils::setMasked(osc.dist_amt, dist_amt_targ, mask);
	}

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
		recalcUnison(unison, unison_mod);
	}
	unison.voices = unison_v;

	bool isFMOutput = audioProcessor.synth->fm->isOut[id].hmax() > 0.f;
	if (!isFMOutput || (osc.level.get(lane) <= 1e-5f && osc.level_targ.get(lane) < 1e-5f))
		unison.voices = 1; // TODO remove this?

	triggered = false;
}

void OSC::recalcUnison(SIMDUnison& unison, int unison_mod) const
{
	alignas(sizeof(SIMDF)) std::array<float, MAX_UNISON> unison_mask;
	for (int i = 0; i < unison.voices; ++i)
		unison_mask[i] = 1.f;

	alignas(sizeof(SIMDF)) auto unison_ratio = Unison::generateDetuneRatios(unison.voices, unison_detune, unison_spread, (Unison::Mode)unison_mod);
	for (int i = 0; i < 4; ++i)
	{
		unison.mask[i].load(&unison_mask[i * 4]);
		unison.ratio[i].load(&unison_ratio[i * 4]);
	}

	auto panarr = Unison::generateVoicesPan(unison.voices, unison_stereo);
	auto gainarr = Unison::generateVoicesGain(unison.voices, unison_blend, true, (Unison::Mode)unison_mod);
	for (int i = 0; i < unison.voices; i += SIMDSZ)
	{
		SIMDF p = mipp::load(&panarr[i]);
		auto lr = Utils::panToGainCheap(p);
		SIMDF gain = mipp::load(&gainarr[i]);
		unison.gain_l[i / SIMDSZ] = lr[0] * gain;
		unison.gain_r[i / SIMDSZ] = lr[1] * gain;
	}
}
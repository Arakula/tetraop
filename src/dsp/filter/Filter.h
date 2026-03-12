// Copyright 2026 tilr
#pragma once

#include <cmath>
#include "../../Globals.h"
#include "../../engine/Utils.h"

using namespace globals;

class Filter
{
public:
	enum Type
	{
		kDigital12,
		kDigital24,
		kAnalog12,
		kAnalog24,
		kLadder12,
		kLadder24,
		kTB303,
		kPhaserPos,
		kPhaserNeg,
		kOff,
	};

	enum Slope
	{
		k12p,
		k24p
	};

	enum Mode
	{
		LP,
		BP,
		HP,
		BS,
		PK
	};

	Type type;
	Mode filterMode{ LP };
	Slope filterSlope{ k12p };

	SIMDF cut = 2000.f;
	SIMDF cut_targ = 2000.f;
	SIMDF res = 0.f;
	SIMDF res_targ = 0.f;

	SIMDF drivenorm = 0.f;
	SIMDF drive = 1.f;
	SIMDF idrive = 1.f;

	SIMDF mix = 1.f;

	std::array<SIMDF, MAX_BLOCKSIZE> out;

	static constexpr float kMinNyquistMult = 0.48f;
	static constexpr float kMaxRads = 0.499f * juce::MathConstants<float>::pi;

	float srate = 44100.f;
	float israte = 0.f;
	float freqScale = 0.f;

	Filter(Type type) : type(type) {}
	virtual ~Filter() {}
	virtual void prepare(float srate_, bool recalc = true)
	{
		srate = srate_;
		israte = 1.f / srate;
		freqScale = MathConstants<float>::pi / srate;
		if (recalc)
			init(cut_targ, res_targ, true, { true, true, true, true });
	}
	virtual void setMode(Mode mode) { filterMode = mode; }
	virtual void setSlope(Slope slope) { filterSlope = slope; }
	virtual void setDrive(SIMDF norm, SIMDM mask) { (void)norm; (void)mask; };

	void setMix(SIMDF m, SIMDM mask)
	{
		Utils::setMasked(mix, m, mask);
	}

	virtual void init(SIMDF cutoff, SIMDF resonance, bool reset, SIMDM mask) = 0;
	virtual void clear(SIMDF sample, SIMDM mask) = 0;
	virtual void processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int startSample, int nsamps, SIMDF mask) = 0;
};
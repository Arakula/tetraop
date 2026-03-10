// Copyright 2026 tilr
#pragma once

#include <cmath>
#include "../../Globals.h"
#include "../../engine/Utils.h"

using namespace globals;

class Filter
{
public:
	// ###################################
	// group = voiceID / SIMD_WIDTH;
	// lane  = voiceID % SIMD_WIDTH;

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
		kPhaserNeg
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

	SIMDF cut = 0.f;
	SIMDF cut_targ = 0.f;
	SIMDF res = 0.f;
	SIMDF res_targ = 0.f;

	std::array<float, MAX_BLOCKSIZE> out{};

	static constexpr float kMinNyquistMult = 0.48f;
	static constexpr float kMaxRads = 0.499f * juce::MathConstants<float>::pi;

	inline static LookupTable tanhLUT = LookupTable(
		[](float x) { return std::tanh(x); },
		-5.0, 5.0, 1024
	);

	inline static LookupTable coeffLUT = LookupTable(
		[] (float ratio) {
			float scaled = ratio * juce::MathConstants<float>::pi;
			return std::tan(std::min(kMaxRads, scaled));
		},
		0.0f, 0.5f, 2048
	);

	float srate = 44100.f;
	float freqScale = 0.f;

	Filter(Type type) : type(type) {}
	virtual ~Filter() {}
	virtual void prepare(float srate_)
	{
		srate = srate_;
		freqScale = MathConstants<float>::pi / srate;
	}
	virtual void setMode(Mode mode) { filterMode = mode; }
	virtual void setSlope(Slope slope) { filterSlope = slope; }
	virtual void setDrive(SIMDF norm, SIMDM mask) { (void)norm; (void)mask; };

	void setTargets(SIMDF cut_, SIMDF res_, SIMDM mask)
	{
		Utils::setMasked(cut_targ, cut_, mask);
		Utils::setMasked(res_targ, res_, mask);
	}

	virtual void init(SIMDF cutoff, SIMDF resonance, bool reset, SIMDM mask) = 0;
	virtual void clear(SIMDF sample, SIMDM mask) = 0;
	virtual void processBlock(float* buf, int startSample, int nsamps, int blocksize, SIMDM mask) = 0;

	inline static float getCoeff(float freq, float srate) {
		freq = jlimit(20.0f, srate * kMinNyquistMult, freq);
		float ratio = jlimit(0.0f, 0.5f, freq / srate);
		return coeffLUT.cubic(ratio);
	}
};
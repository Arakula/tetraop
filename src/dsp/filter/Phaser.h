// Copyright 2025 tilr
// Based off Vital synth phaser filter
#pragma once

#include <JuceHeader.h>
#include "Filter.h"
#include "OnePole.h"
#include "../../engine/Utils.h"
#include <cmath>

class Phaser : public Filter
{

public:
	static constexpr int kPeakStage = 4;
	static constexpr int kMaxStages = 3 * kPeakStage;
	static constexpr float kClearRatio = 20.0f;

	SIMDF morph = 0.f;

	Phaser(bool pos) : Filter(pos ? kPhaserPos : kPhaserNeg) {}
	~Phaser(){}

	void init(SIMDF cut, SIMDF res, bool reset, SIMDM mask) override;
	void clear(SIMDF sample, SIMDM mask) override;
	void setDrive(SIMDF drive, SIMDM mask) override;
	void processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, SIMDF mask) override;

	template<Type>
	void _processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, SIMDF mask);

private:
	OnePole remove_lows_stage;
	OnePole remove_highs_stage;
	OnePole stages[kMaxStages];
	SIMDF allpass_output = 0.0f;

	const SIMDF one = 1.f;
	SIMDF g = 0.0;
	SIMDF g_targ = 0.0;
	SIMDF g_step = 0.0;
	SIMDF k = 0.0;
	SIMDF k_targ = 0.0;
	SIMDF k_step = 0.0;
};
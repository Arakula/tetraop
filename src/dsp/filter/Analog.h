// Copyright 2025 tilr
// Port of Vital Sallen-Key filter
#pragma once

#include <JuceHeader.h>
#include "Filter.h"
#include "OnePole.h"
#include "../../engine/Utils.h"

class Analog : public Filter
{

public:
	static constexpr float kDriveResonanceBoost = 2.0f;

	Analog(Slope p) : Filter(p == k12p ? kAnalog12 : kAnalog24) {}
	~Analog(){}

	void init(SIMDF cutoff, SIMDF resonance, bool reset, SIMDM mask) override;
	void clear(SIMDF sample, SIMDM mask) override;
	void setDrive(SIMDF drive, SIMDM mask) override;
	void processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, int blocksize, SIMDF mask) override;

	template<Mode, Slope>
	void _processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, int blocksize, SIMDF mask);


private:
	OnePole pre_stage1;
	OnePole pre_stage2;
	OnePole stage1;
	OnePole stage2;

	SIMDF g = 0.f;
	SIMDF g_targ = 0.f;
	SIMDF g_step = 0.f;
	SIMDF k = 0.f;
	SIMDF k_targ = 0.f;
	SIMDF k_step = 0.f;
};
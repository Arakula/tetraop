// Copyright 2025 tilr
// Based off Saikes Yutani bass filters
// https://github.com/JoepVanlier/JSFX/blob/master/Yutani/Yutani_Dependencies/Saike_Yutani_Filters.jsfx-inc
#pragma once

#include <JuceHeader.h>
#include "Filter.h"
#include "../../engine/Utils.h"
#include <cmath>

class Digital : public Filter
{
public:
	inline static SIMDF hardTanh(SIMDF value, SIMDM mask) {
		static constexpr float kHardness = 0.66f;
		static constexpr float kHardnessInv = 1.0f - kHardness;
		static constexpr float kHardnessInvRec = 1.0f / kHardnessInv;

		auto clamped = value.sat(-kHardness, kHardness);
		auto val = clamped + ((value - clamped) * kHardnessInvRec).tanh() * (SIMDF(1.0f) - kHardness);
		return val.blend(value, mask);
	}

	Digital(Slope p) : Filter(p == k12p ? kDigital12 : kDigital24) {}
	~Digital(){}

	void init(SIMDF cutoff, SIMDF resonance, bool reset, SIMDM mask) override;
	void clear(SIMDF sample, SIMDM mask) override;
	void setDrive(SIMDF drive, SIMDM mask) override;
	void processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, int blockoffset, int blocksize, SIMDF mask) override;

	template<Mode, Slope>
	void _processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, int blockoffset, int blocksize, SIMDF mask);

private:
	SIMDF ic1 = 0.f;
	SIMDF ic2 = 0.f;
	SIMDF ic3 = 0.f;
	SIMDF ic4 = 0.f;

	SIMDF g = 0.f;
	SIMDF g_targ = 0.f;
	SIMDF g_step = 0.f;
	SIMDF k = 0.f;
	SIMDF k_targ = 0.f;
	SIMDF k_step = 0.f;
	SIMDF a1 = 0.f;
	SIMDF a2 = 0.f;
	SIMDF a3 = 0.f;

	SIMDF b0 = 0.f;
	SIMDF b1 = 0.f;
	SIMDF b2 = 0.f;
	SIMDF x1 = 0.f;
	SIMDF x2 = 0.f;
	SIMDF y1 = 0.f;
	SIMDF y2 = 0.f;
};
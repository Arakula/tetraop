// Copyright 2026 tilr
// Port of Rosic Open303 https://github.com/maddanio/open303

#pragma once

#include <JuceHeader.h>
#include "Filter.h"
#include "../../engine/Utils.h"
#include <cmath>

class TB303 : public Filter
{
public:
	TB303() : Filter(kTB303) 
	{ 
		highpass.setCutoff(150.f); 
		clear(0.f, { true, true, true, true });
	}
	~TB303(){}

	virtual void prepare(float srate_, bool recalc = true)
	{
		Filter::prepare(srate_, recalc);
		highpass.setSampleRate(srate);
	}

	inline SIMDF saturate(SIMDF x)
	{
		static constexpr float r6 = 1.0f / 6.0f;
		static constexpr float SQRT2 = 1.4142135623730951f;
		x = x.sat(-SQRT2, SQRT2);
		return x - x * x * x * r6;
	}

	void init(SIMDF cutoff, SIMDF resonance, bool reset, SIMDM mask) override;
	void clear(SIMDF sample, SIMDM mask) override;
	void setDrive(SIMDF drive, SIMDM mask) override;
	void processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, SIMDF mask) override;

	template<Mode>
	void _processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, SIMDF mask);

private:
	SIMDCBlocker highpass{};
	SIMDF one = 1.f;
	SIMDF b0 = 0.f; 
	SIMDF a1 = 0.f;              // coefficients for the first order sections
	SIMDF y1 = 0.f; 
	SIMDF y2 = 0.f; 
	SIMDF y3 = 0.f; 
	SIMDF y4 = 0.f;      // output signals of the 4 filter stages
    SIMDF c0, c1, c2, c3, c4;  // coefficients for combining various ouput stages
    SIMDF k = 0.f;                   // feedback factor in the loop
    SIMDF g = 0.f;                   // output gain
	SIMDF b0_targ = 0.f;
	SIMDF b0_step = 0.f;
	SIMDF a1_targ = 0.f; 
	SIMDF a1_step = 0.f;
	SIMDF k_targ = 0.f;
	SIMDF k_step = 0.f;
};
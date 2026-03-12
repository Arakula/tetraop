// Copyright 2025 tilr
// An adapted version of the JUCE Ladder filter
#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include "Filter.h"
#include <cmath>

class Ladder : public Filter
{
public:
	Ladder(Slope p) : Filter(p == k12p ? kLadder12 : kLadder24) {}
	~Ladder(){}

	void init(SIMDF cutoff, SIMDF resonance, bool reset, SIMDM mask) override;
	void clear(SIMDF sample, SIMDM mask) override;
	void setDrive(SIMDF norm, SIMDM mask) override;
	void setMode(Mode mode_) override;
	void updateState();

	void processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, int blocksize, SIMDF mask) override;

	template<Mode, Slope>
	void _processBlock(std::array<SIMDF, MAX_BLOCKSIZE>& input, int start, int nsamps, int blocksize, SIMDF mask);

private:
	SIMDF drive2 = 1.0;
	SIMDF gain = 1.0;
	SIMDF gain2 = 1.0;
	SIMDF comp = 0.0;

	SIMDF f0 = 0.f;
	SIMDF f0_targ = 0.f;
	SIMDF f0_step = 0.f;
	SIMDF k = 0.f;
	SIMDF k_targ = 0.f;
	SIMDF k_step = 0.f;

	static constexpr int numStates = 5;
	std::array<SIMDF, numStates> state = {0.0, 0.0, 0.0, 0.0, 0.0};
	std::array<SIMDF, numStates> A = {0.0, 0.0, 0.0, 0.0, 0.0};
};
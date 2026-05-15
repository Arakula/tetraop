// Copyright (C) 2025 tilr
#pragma once
#include "FX.h"
#include "../../engine/Modulation.h"

class Compressor : public FX
{
public:
	Compressor(RipplerAudioProcessor& p, int layer);
	~Compressor() override;

	void prepare(float _srate) override;
	void processBlock(float* left, float* right, int nsamps, int blockoffset, bool audioRate) override;
	void clear() override {}

private:
	float srate = 44100.f;
	float t = 0.f;
	float seekGain = 1.f;
	float gain = 1.f;

	std::atomic<float>* makeupParam = nullptr;
	Modulation::Param* threshParam = nullptr;
	Modulation::Param* ratioParam = nullptr;
	Modulation::Param* attackParam = nullptr;
	Modulation::Param* releaseParam = nullptr;
	Modulation::Param* gainParam = nullptr;
};

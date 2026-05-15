// Copyright (C) 2025 tilr
#pragma once
#include "FX.h"
#include <functional>
#include "../SVF.h"
#include "../../engine/Modulation.h"

class Distortion 
	: public FX
	, private juce::AudioProcessorValueTreeState::Listener
{
public:
	Distortion(RipplerAudioProcessor& p, int layer);
    ~Distortion() override;

	void parameterChanged(const juce::String& paramId, float value) override;
	void prepare(float _srate) override;
	void processBlock(float* left, float* right, int nsamps, int blockoffset, bool audioRate) override;
	void clear() override {}

private:
	bool queueReset = false;
	float drive = 0.f;
	float gain = 0.f;
	float mix = 0.f;

	float srate = 88200.f;
	SVF filterL{};
	SVF filterR{};
	SVF colorL{};
	SVF colorR{};
	std::vector<float> dryLeft;
	std::vector<float> dryRight;

	std::atomic<float>* modeParam = nullptr;
	Modulation::Param* driveParam = nullptr;
	Modulation::Param* gainParam = nullptr;
	Modulation::Param* mixParam = nullptr;
	Modulation::Param* filterParam = nullptr;
	Modulation::Param* colorParam = nullptr;
};

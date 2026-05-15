// Copyright (C) 2025 tilr
#pragma once
#include "FX.h"
#include "../SVF.h"
#include "../../engine/Modulation.h"

class EQ 
	: public FX
	, private juce::AudioProcessorValueTreeState::Listener
{
public:
	EQ(RipplerAudioProcessor& p, int layer);
	~EQ() override;

	void parameterChanged(const juce::String& paramId, float value) override;
	void prepare(float _srate) override;
	void processBlock(float* left, float* right, int nsamps, int blockoffset, bool audioRate) override;
	void clear() override;
	void initFilters();

private:
	float srate = 44100.f;

	std::array<SVF, globals::EQ_BANDS> bandsL;
	std::array<SVF, globals::EQ_BANDS> bandsR;
	std::array<std::atomic<float>*, globals::EQ_BANDS> freqParams{};
	std::array<std::atomic<float>*, globals::EQ_BANDS> gainParams{};
	std::array<std::atomic<float>*, globals::EQ_BANDS> qParams{};
	std::array<std::atomic<float>*, globals::EQ_BANDS> modeParams{};
	std::array<Modulation::Param*, globals::EQ_BANDS> freqModParams{};
	std::array<Modulation::Param*, globals::EQ_BANDS> gainModParams{};
	std::array<Modulation::Param*, globals::EQ_BANDS> qModParams{};
};

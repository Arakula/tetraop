// Copyright (C) 2025 tilr
#pragma once
#include "FX.h"
#include "../DelayLine.h"
#include "../../engine/Utils.h"
#include "../../engine/Modulation.h"

struct BlockTimer
{
	void add ( double us ) noexcept
	{
		totalUs += us;
		maxUs = juce::jmax ( maxUs, us );
		minUs = ( count == 0 ? us : juce::jmin ( minUs, us ) );
		++count;
	}

	void reset () noexcept
	{
		totalUs = 0.0;
		maxUs = 0.0;
		minUs = 0.0;
		count = 0;
	}

	double totalUs = 0.0;
	double maxUs = 0.0;
	double minUs = 0.0;
	int    count = 0;
};

class Chorus : public FX
{
public:
	static constexpr int MAX_VOICES = 16;

	Chorus(RipplerAudioProcessor& p, int _layer);
	~Chorus() override {}

	void prepare(float _srate) override;
	void processBlock(float* left, float* right, int nsamps, int blockoffset, bool audioRate) override;
	void clear() override 
	{
		for (auto& delay : delay_l) delay.clear();
		for (auto& delay : delay_r) delay.clear();
		lpstate_l = 0.f;
		lpstate_r = 0.f;
		hpstate_l = 0.f;
		hpstate_r = 0.f;
		phase = 0.f;
	}

private:
	int voices = 0;
	float delay_samples = 0.f;
	std::array<float, MAX_VOICES> base_delay{};
	std::array<float, MAX_VOICES> phase_offset{};
	std::array<float, MAX_VOICES> phase_offset_sin{};
	std::array<float, MAX_VOICES> phase_offset_cos{};
	float jitter_samples = 0.f;
	float srate = 44100.f;
	std::array<DelayLine, MAX_VOICES> delay_l{};
	std::array<DelayLine, MAX_VOICES> delay_r{};
	float phase = 0.f;
	float rate = 0.f;
	float depth = 0.f;
	float mix = 0.5f;
	float hpfreq = 0.f;
	float lpfreq = 0.f;
	float feedback = 0.f;

	// one pole filter states
	float lpstate_l = 0.f;
	float lpstate_r = 0.f;
	float hpstate_l = 0.f;
	float hpstate_r = 0.f;

	BlockTimer chorusTimer;

	std::atomic<float>* voicesParam = nullptr;
	std::atomic<float>* rateParam = nullptr;
	std::atomic<float>* mixParam = nullptr;
	std::atomic<float>* depthParam = nullptr;
	std::atomic<float>* highcutParam = nullptr;
	std::atomic<float>* lowcutParam = nullptr;
	std::atomic<float>* feedbackParam = nullptr;

	Modulation::Param* rateModParam = nullptr;
	Modulation::Param* mixModParam = nullptr;
	Modulation::Param* depthModParam = nullptr;
	Modulation::Param* highcutModParam = nullptr;
	Modulation::Param* lowcutModParam = nullptr;
	Modulation::Param* feedbackModParam = nullptr;
};

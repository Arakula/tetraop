// Copyright (C) 2025 tilr
#pragma once
#include <JuceHeader.h>
#include "../DelayLine.h"
#include "../../engine/Utils.h"
#include "../../engine/Modulation.h"
#include "FX.h"

class Delay 
	: public FX
	, private juce::AudioProcessorValueTreeState::Listener
{
public:
	Delay(RipplerAudioProcessor& p, int layer);
	~Delay() override;

	void parameterChanged(const juce::String& paramId, float value) override;
	void prepare(float _srate) override;

	std::array<int, 2> getTimeSamples(bool forceSync, int blockoffset);
	void processBlock(float* left, float* right, int nsamps, int blockoffset, bool audioRate) override;
	void clear() override 
	{
		predelayL.clear();
		predelayR.clear();
		delayL.clear();
		delayR.clear();
		lpstate_l = 0.f;
		lpstate_r = 0.f;
		hpstate_l = 0.f;
		hpstate_r = 0.f;
        auto time = getTimeSamples ( false, 0 );
        timeL.reset((float)time[0]);
		timeR.reset((float)time[1]);
	}

private:
	RCFilter timeL{};
	RCFilter timeR{};
	DelayLine predelayL{};
	DelayLine predelayR{};
	DelayLine delayL{};
	DelayLine delayR{};
	float srate = 44100.f;

	float lpfreq = 0.f;
	float hpfreq = 0.f;
	float lpstate_l = 0.f;
	float lpstate_r = 0.f;
	float hpstate_l = 0.f;
	float hpstate_r = 0.f;

	std::atomic<float>* modeParam = nullptr;
	std::atomic<float>* linkParam = nullptr;
	std::atomic<float>* syncLParam = nullptr;
	std::atomic<float>* syncRParam = nullptr;
	std::atomic<float>* highcutParam = nullptr;
	std::atomic<float>* lowcutParam = nullptr;

	Modulation::Param* rateLParam = nullptr;
	Modulation::Param* rateRParam = nullptr;
	Modulation::Param* rateSyncLParam = nullptr;
	Modulation::Param* rateSyncRParam = nullptr;
	Modulation::Param* feedbackParam = nullptr;
	Modulation::Param* pipoWidthParam = nullptr;
	Modulation::Param* mixParam = nullptr;
	Modulation::Param* highcutModParam = nullptr;
	Modulation::Param* lowcutModParam = nullptr;
};

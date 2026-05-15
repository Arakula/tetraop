
#pragma once

#include "FX.h"
#include "../../engine/Modulation.h"
#include "./MiniVerb.h"

class ReverbFX
	: public FX
	, private juce::AudioProcessorValueTreeState::Listener
{
public:
	ReverbFX(RipplerAudioProcessor& p, int layer);
	~ReverbFX() override;

	void parameterChanged(const juce::String& paramId, float value) override;
	void prepare(float _srate) override;
	void processBlock(float* left, float* right, int nsamps, int blockoffset, bool audioRate) override;
	void clear() override;

private:
    float srate = 44100.0f;
    std::unique_ptr<MiniVerb> miniVerb;

	std::atomic<float>* modeParam = nullptr;
};

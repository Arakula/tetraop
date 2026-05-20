
#pragma once

#include "FX.h"
#include "../../engine/Modulation.h"
#include "./TetraVerb.h"

class ReverbFX
	: public FX
	, private juce::AudioProcessorValueTreeState::Listener
{
public:
	ReverbFX(TetraOPAudioProcessor& p);
	~ReverbFX() override;

	void parameterChanged(const juce::String& paramId, float value) override;
	void prepare(float _srate) override;
	void processBlock(float* left, float* right, int nsamps, int blockoffset, bool audioRate) override;
	void clear() override;

private:
    float srate = 44100.0f;
    std::unique_ptr<TetraVerb> tetraVerb;

	std::atomic<float>* modeParam = nullptr;
};

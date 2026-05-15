#pragma once
#include <JuceHeader.h>
#include "UIFX.h"
#include "../../Globals.h"
#include "../widgets/Rotary.h"
#include "../UIUtils.h"

class TetraOPAudioProcessorEditor;

class FXPhaser
    : public UIFX
    , private juce::AudioProcessorValueTreeState::Listener
{
public:
	juce::TextButton syncBtn { "sync mode" };

	std::unique_ptr<Rotary> center;
	std::unique_ptr<Rotary> depth;
	std::unique_ptr<Rotary> rate;
	std::unique_ptr<Rotary> rateSync;
	std::unique_ptr<Rotary> res;
	std::unique_ptr<Rotary> morph;
	std::unique_ptr<Rotary> stereo;
	std::unique_ptr<Rotary> mix;

	FXPhaser(TetraOPAudioProcessorEditor& e);
	~FXPhaser() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
	void paint(juce::Graphics& g) override;
	void resized() override;
    void toggleUIComponents();
    void showSyncMenu();
};

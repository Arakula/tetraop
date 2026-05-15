#pragma once
#include <JuceHeader.h>
#include "UIFX.h"
#include "../../Globals.h"
#include "../widgets/Rotary.h"

class TetraOPAudioProcessorEditor;

class FXReverb
	: public UIFX
	, private juce::AudioProcessorValueTreeState::Listener
{
public:
	std::unique_ptr<Rotary> predel;
	std::unique_ptr<Rotary> decay;
	std::unique_ptr<Rotary> size;
	std::unique_ptr<Rotary> damp;
	std::unique_ptr<Rotary> lowcut;
	std::unique_ptr<Rotary> highcut;
	std::unique_ptr<Rotary> modrate;
	std::unique_ptr<Rotary> moddepth;
	std::unique_ptr<Rotary> mix;
	juce::TextButton modeBtn;

	FXReverb(TetraOPAudioProcessorEditor& e);
	~FXReverb() override;

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(juce::Graphics& g) override;
	void resized() override;
    void toggleUIComponents();
	void showModeMenu();
};

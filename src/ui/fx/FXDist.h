#pragma once
#include <JuceHeader.h>
#include "UIFX.h"
#include "../../Globals.h"
#include "../widgets/Rotary.h"

class TetraOPAudioProcessorEditor;

class FXDist
	: public UIFX
	, private juce::AudioProcessorValueTreeState::Listener
{
public:
	std::unique_ptr<Rotary> filter;
	std::unique_ptr<Rotary> drive;
	std::unique_ptr<Rotary> color;
	std::unique_ptr<Rotary> gain;
	std::unique_ptr<Rotary> mix;

	juce::TextButton modeBtn;

	FXDist(TetraOPAudioProcessorEditor& e);
	~FXDist() override;

	void parameterChanged(const juce::String& parameterID, float newValue) override;
	void onActiveToggle() override;

	void mouseDown(const juce::MouseEvent& e) override;

    void paint(juce::Graphics& g) override;
	void resized() override;
	void toggleUIComponents();
	void showModeMenu();
};

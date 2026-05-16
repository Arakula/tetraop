#pragma once
#include <JuceHeader.h>
#include "UIFX.h"
#include "../../Globals.h"
#include "../widgets/Rotary.h"

class TetraOPAudioProcessorEditor;

class FXCompressor
	: public UIFX
	, private juce::AudioProcessorValueTreeState::Listener
	, private juce::Timer
{
public:
	std::unique_ptr<Rotary> thresh;
	std::unique_ptr<Rotary> ratio;
	std::unique_ptr<Rotary> attack;
	std::unique_ptr<Rotary> release;
	std::unique_ptr<Rotary> gain;

	juce::TextButton makeupBtn;

	FXCompressor(TetraOPAudioProcessorEditor& e);
	~FXCompressor() override;

	void timerCallback() override;
	void parameterChanged(const juce::String& parameterID, float newValue) override;

	void onActiveToggle() override;
	void mouseDown(const juce::MouseEvent& e) override;
    void paint(juce::Graphics& g) override;
	void resized() override;
	void toggleUIComponents();

private:
	Rectangle<int> redBounds;
};

#pragma once
#include <JuceHeader.h>
#include "UIFX.h"
#include "../../Globals.h"
#include "../widgets/Rotary.h"
#include "../widgets/TimePicker.h"
#include "../widgets/HSlider.h"

class TetraOPAudioProcessorEditor;

class FXDelay
	: public UIFX
	, private juce::AudioProcessorValueTreeState::Listener
{
public:
	std::unique_ptr<Rotary> feedback;
	std::unique_ptr<Rotary> lowcut;
	std::unique_ptr<Rotary> highcut;
	std::unique_ptr<Rotary> mix;
	std::unique_ptr<HSlider> width;
	std::unique_ptr<TimePicker> rateL;
	std::unique_ptr<TimePicker> rateSyncL;
	std::unique_ptr<TimePicker> rateR;
	std::unique_ptr<TimePicker> rateSyncR;

	juce::TextButton delayMode;
	juce::TextButton syncModeLBtn;
	juce::TextButton syncModeRBtn;
	juce::TextButton linkBtn;
	juce::TextButton modeBtn;

	FXDelay(TetraOPAudioProcessorEditor& e);
	~FXDelay() override;

	void parameterChanged(const juce::String& parameterID, float newValue) override;
	void mouseDown(const juce::MouseEvent& e) override;

    void paint(juce::Graphics& g) override;
	void resized() override;
	void toggleUIComponents();
	void showSyncMenu(bool isLeft);
	void showModeMenu();
};

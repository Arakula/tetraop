#pragma once
#include <JuceHeader.h>
#include "UIFX.h"
#include "../../Globals.h"
#include "../widgets/Rotary.h"

class TetraOPAudioProcessorEditor;

class FXChorus : public UIFX
{
public:
	std::unique_ptr<Rotary> rate;
	std::unique_ptr<Rotary> depth;
	std::unique_ptr<Rotary> lowcut;
	std::unique_ptr<Rotary> highcut;
	std::unique_ptr<Rotary> feedback;
	std::unique_ptr<Rotary> mix;

	FXChorus(TetraOPAudioProcessorEditor& e);
	~FXChorus() override;

	void mouseDown(const juce::MouseEvent& e) override;

    void paint(juce::Graphics& g) override;
	void resized() override;
	void showVoicesMenu();

private:
	juce::Rectangle<float> voiceBounds;
};

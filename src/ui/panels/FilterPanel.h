#pragma once

#include <JuceHeader.h>
#include "../widgets/Rotary.h"
#include "../../dsp/filter/Filter.h"

class TetraOPAudioProcessorEditor;

class FilterPanel
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
{
public:

    FilterPanel(TetraOPAudioProcessorEditor& e, int fid);
    ~FilterPanel() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(Graphics& g) override;
    void resized() override;
    void toggleUIComponents();
	void showTypeMenu();
	void showModeMenu();

	TextButton onBtn;
	TextButton inaBtn;
	TextButton inbBtn;
	TextButton incBtn;
	TextButton indBtn;
	TextButton inf1Btn;
	TextButton typeBtn;
	TextButton modeBtn;
	TextButton ktrackBtn;

	std::unique_ptr<Rotary> cut;
	std::unique_ptr<Rotary> res;
	std::unique_ptr<Rotary> drive;
	std::unique_ptr<Rotary> mix;

private:
	int fid = 0;
    String prefix;
    TetraOPAudioProcessorEditor& editor;
};
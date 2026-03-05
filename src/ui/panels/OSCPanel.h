#pragma once

#include <JuceHeader.h>
#include "../widgets/Rotary.h"

class TetraOPAudioProcessorEditor;

class OSCPanel
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
{
public:

    OSCPanel(TetraOPAudioProcessorEditor& e, int oscId);
    ~OSCPanel() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(Graphics& g) override;
    void resized() override;
    void toggleUIComponents();

	std::unique_ptr<Rotary> level;
	std::unique_ptr<Rotary> pan;
	std::unique_ptr<Rotary> phase;
	std::unique_ptr<Rotary> rand;
	std::unique_ptr<Rotary> morph;
	std::unique_ptr<Rotary> dist;
	std::unique_ptr<Rotary> detune;
	std::unique_ptr<Rotary> blend;
	std::unique_ptr<Rotary> wide;
	std::unique_ptr<Rotary> semis;
	std::unique_ptr<Rotary> cents;

private:
	int oscId;
    String prefix;
    TetraOPAudioProcessorEditor& editor;
};
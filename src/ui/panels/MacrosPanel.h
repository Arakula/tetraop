#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"
#include "../widgets/Rotary.h"
#include "../widgets/LayoutPicker.h"

class TetraOPAudioProcessorEditor;
using namespace globals;

class MacrosPanel
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
{
public:

	std::unique_ptr<LayoutPicker> layout;

    MacrosPanel(TetraOPAudioProcessorEditor& e);
    ~MacrosPanel() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(Graphics& g) override;
    void resized() override;
    void toggleUIComponents();

private:
    TetraOPAudioProcessorEditor& editor;
};
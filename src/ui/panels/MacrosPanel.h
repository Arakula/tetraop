#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"
#include "../widgets/Rotary.h"
#include "../widgets/LayoutPicker.h"
#include "../widgets/Macro.h"

class TetraOPAudioProcessorEditor;
using namespace globals;

class MacrosPanel
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
{
public:

    std::vector<std::unique_ptr<Macro>> macros;

    MacrosPanel(TetraOPAudioProcessorEditor& e);
    ~MacrosPanel() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(Graphics& g) override;
    void resized() override;
    void toggleUIComponents();

private:
    TetraOPAudioProcessorEditor& editor;
};
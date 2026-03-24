#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"
#include "../widgets/Rotary.h"

class TetraOPAudioProcessorEditor;
using namespace globals;

class FmMatrixPanel
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
{
public:

    FmMatrixPanel(TetraOPAudioProcessorEditor& e);
    ~FmMatrixPanel() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(Graphics& g) override;
    void resized() override;
    void toggleUIComponents();

private:
    TetraOPAudioProcessorEditor& editor;
};
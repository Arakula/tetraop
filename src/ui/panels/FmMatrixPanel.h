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

    TextButton closeBtn { "close" };
    TextButton fmBtn{ "fm" };
    TextButton rmBtn{ "rm" };
    std::unique_ptr<Rotary> fm[16];
    std::unique_ptr<Rotary> rm[16];
    std::unique_ptr<Rotary> out[4];

    FmMatrixPanel(TetraOPAudioProcessorEditor& e);
    ~FmMatrixPanel() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(Graphics& g) override;
    void resized() override;
    void toggleUIComponents();

private:
    TetraOPAudioProcessorEditor& editor;
};
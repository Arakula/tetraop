#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"
#include "../widgets/Rotary.h"
#include "../widgets/LayoutPicker.h"
#include "../widgets/Modulator.h"

class TetraOPAudioProcessorEditor;
using namespace globals;

class ModulatorsPanel
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
{
public:

    std::unique_ptr<Modulator> vel;
    std::unique_ptr<Modulator> key;
    std::unique_ptr<Modulator> at;
    std::unique_ptr<Modulator> rand;
    std::unique_ptr<Modulator> mwheel;

    ModulatorsPanel(TetraOPAudioProcessorEditor& e);
    ~ModulatorsPanel() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(Graphics& g) override;
    void resized() override;
    void toggleUIComponents();

private:
    TetraOPAudioProcessorEditor& editor;
};
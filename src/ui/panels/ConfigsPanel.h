#pragma once

#include <JuceHeader.h>
#include "../UIUtils.h"
#include "../widgets/CurveEditor.h"
#include "../widgets/ValuePicker.h"
#include "../../Globals.h"
#include "../ScaledPluginEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>

using namespace globals;
class TetraOPAudioProcessorEditor;

class ConfigsPanel
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
    , private juce::Timer
{
public:

    ConfigsPanel(TetraOPAudioProcessorEditor& e);
    ~ConfigsPanel() override;

    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    void toggleUIComponents();
    void showScaleMenu();

    juce::TextButton scaleBtn;
    juce::TextButton mpeEnabledBtn;
    juce::TextButton unboundedMouseBtn;
    std::unique_ptr<CurveEditor> velEditor;
    std::unique_ptr<ValuePicker> bendPicker;

private:
    TetraOPAudioProcessorEditor& editor;
    float currscale;
};

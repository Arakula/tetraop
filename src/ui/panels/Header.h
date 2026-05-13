#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"
#include "../widgets/Rotary.h"

class TetraOPAudioProcessorEditor;
using namespace globals;

class CPUMeter
    : public juce::Component
    , private juce::Timer
{
public:
    TetraOPAudioProcessorEditor& editor;
    CPUMeter(TetraOPAudioProcessorEditor& p) : editor(p)
    {
        startTimerHz(30);
    };

    void timerCallback() override
    {
        repaint();
    }

    void paint(Graphics& g) override;
};

class Header
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
{
public:

    TextButton logo { "logo" };
    TextButton lib { "library" };
    TextButton syn { "synth" };
    TextButton fxs { "effects" };
    TextButton mod { "modmatrix" };
    TextButton cfg { "configs" };
    TextButton prevPreset;
    TextButton nextPreset;
    TextButton preset;
    TextButton saveBtn;
    TextButton menuBtn;
    std::unique_ptr<CPUMeter> cpuMeter;

	std::unique_ptr<Rotary> gain;

    Header(TetraOPAudioProcessorEditor& e);
    ~Header() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(Graphics& g) override;
    void resized() override;
    void toggleUIComponents();

private:
    TetraOPAudioProcessorEditor& editor;
};
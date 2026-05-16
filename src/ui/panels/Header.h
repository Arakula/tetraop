#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"
#include "../UIUtils.h"
#include "../widgets/Rotary.h"
#include "../widgets/Meter.h"
#include "../../engine/PresetManager.h"

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
    , private juce::Timer
{
public:

    TextButton logo { "logo" };
    TextButton lib { "library" };
    TextButton syn { "synth" };
    TextButton fxs { "effects" };
    TextButton mod { "modmatrix" };
    TextButton cfg { "configs" };
    TextButton undoBtn{ "undo" };
    TextButton redoBtn{ "redo" };
    TextButton prevPreset;
    TextButton nextPreset;
    TextButton preset;
    TextButton saveBtn;
    TextButton menuBtn;
    std::unique_ptr<CPUMeter> cpuMeter;

	std::unique_ptr<Rotary> gain;
	std::unique_ptr<Meter> meter;

    Header(TetraOPAudioProcessorEditor& e);
    ~Header() override;

    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(Graphics& g) override;
    void resized() override;
    void toggleUIComponents();
    void showPresets();
    void savePreset();
    void selectNextPreset(bool isNext);

private:
    std::unique_ptr<juce::FileChooser> fileChooser;
    TetraOPAudioProcessorEditor& editor;
};
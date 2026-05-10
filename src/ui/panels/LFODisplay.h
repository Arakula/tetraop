#pragma once

#include <JuceHeader.h>
#include "../widgets/Rotary.h"
#include "../../dsp/Pattern.h"
#include "../UIUtils.h"
#include "../widgets/CurveEditor.h"
#include "../widgets/Modulator.h"

class TetraOPAudioProcessorEditor;

class LFODisplay
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
    , private juce::Timer
{
public:
    std::vector<std::unique_ptr<Modulator>> lfos;

    juce::TextButton gridBtn;
    juce::TextButton rotLeftBtn;
    juce::TextButton rotRightBtn;
    juce::TextButton roundBtn;
    juce::TextButton fileBtn;
    juce::TextButton modeBtn;

    std::unique_ptr<Rotary> rate;
    std::unique_ptr<Rotary> rateSync;
    std::unique_ptr<Rotary> smooth;
    std::unique_ptr<Rotary> delay;
    std::unique_ptr<Rotary> delaySync;
    std::unique_ptr<Rotary> rise;
    std::unique_ptr<Rotary> riseSync;
    juce::TextButton syncBtn;
    std::unique_ptr<CurveEditor> display;

    LFODisplay(TetraOPAudioProcessorEditor& e);
    ~LFODisplay() override;

    void mouseDown(const juce::MouseEvent& e) override;

    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void connect(juce::String modid);
    void disconnect();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void toggleUIComponents();
    void showGridMenu();
    void showModeMenu();
    void showSyncMenu();
    void showFileMenu();
    void buildFileSubmenu(juce::PopupMenu& menu, const juce::File& folder);
    void loadLfoFile(const juce::File& file);
    void saveLfoFile(const juce::String& name);

private:
    juce::Rectangle<float> viewBounds{};
    TetraOPAudioProcessorEditor& editor;
    juce::String lfoid = "";
    int lfoidx = 0;
    bool isSync = false;
    bool isActive = false;
};
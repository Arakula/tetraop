#pragma once

#include <JuceHeader.h>
#include "../UIUtils.h"
#include "../ModulatedParam.h"
#include "../ModulationDragSource.h"
#include "Rotary.h"

class TetraOPAudioProcessorEditor;

class Macro
    : public juce::Component
    , public ModulationDragSource
    , private juce::Timer
{
public:
    std::unique_ptr<Rotary>rotary;

    Macro(TetraOPAudioProcessorEditor& e, int index);
    ~Macro() override;

    void timerCallback() override;

    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;
    void paint(juce::Graphics& g) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    juce::Point<float> getDragSource() override;

    int index;
private:
    TetraOPAudioProcessorEditor& editor;
    juce::String macroId;
    juce::String macroIdx;
    bool selected = false;
    float lpad = 20.f;
    int connections = 0;
    juce::String macroName = "";
    juce::TextButton nameBtn;
};

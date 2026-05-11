#pragma once

#include <JuceHeader.h>
#include <functional>
#include "../../Globals.h"
#include "../../engine/FmMatrix.h"

using namespace globals;
class TetraOPAudioProcessorEditor;

class LayoutPickerWidget : public juce::Component
{
public:
    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;

    std::function<void(int)> onClick = nullptr;
    int hoverx = -1;
    int hovery = -1;
};

class LayoutPicker
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
{
public:
    TextButton editBtn{ "edit" };

    LayoutPicker(TetraOPAudioProcessorEditor& p);
    ~LayoutPicker() override;
    void parameterChanged(const juce::String& parameterID, float value) override;

    void mouseEnter(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void mouseDown(const MouseEvent& e) override;
	void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    void resized() override;
    void paint(Graphics& g) override;

private:
    TetraOPAudioProcessorEditor& editor;
};
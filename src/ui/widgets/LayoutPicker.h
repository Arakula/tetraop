#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"
#include "../../engine/FmMatrix.h"

using namespace globals;
class TetraOPAudioProcessorEditor;

class LayoutPicker
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
{
public:
    LayoutPicker(TetraOPAudioProcessorEditor& p);
    ~LayoutPicker() override;
    void parameterChanged(const juce::String& parameterID, float value) override;

    void mouseDown(const MouseEvent& e) override;
	void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    void paint(Graphics& g) override;

private:
    TetraOPAudioProcessorEditor& editor;
};
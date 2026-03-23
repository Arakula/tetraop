#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"

using namespace globals;
class TetraOPAudioProcessorEditor;

class ValuePicker
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
{
public:
	float fontSize = 16.f;
	bool isInteger = false;
	String suffix = "";
	String prefix = "";
	int precision = 2;
    bool isPercentage = false;
    Colour color = Colours::white;
    Justification::Flags align = Justification::centred;

    ValuePicker(TetraOPAudioProcessorEditor& p, String paramId);
    ~ValuePicker() override;
    void parameterChanged(const juce::String& parameterID, float newValue);

    void setParam(String id);

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
	void mouseDoubleClick(const juce::MouseEvent&) override;
	void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    void paint(Graphics& g) override;

private:
	String paramId;
    TetraOPAudioProcessorEditor& editor;

	float pixels_per_percent{100.0f};
    float cur_normed_value{0.0f};
    juce::Point<int> last_mouse_position;
    juce::Point<int> start_mouse_pos;
    bool mouse_down = false;
};
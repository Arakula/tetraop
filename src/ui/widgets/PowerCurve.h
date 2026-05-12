#pragma once
#include <JuceHeader.h>
#include "../../Globals.h"
#include "../ModulatedParam.h"

using namespace globals;
class TetraOPAudioProcessorEditor;

class PowerCurve
    : public ModulatedParam
    , private juce::AudioProcessorValueTreeState::Listener
{
public:

    PowerCurve(TetraOPAudioProcessorEditor& e, juce::String pid, bool bipolar);
    ~PowerCurve() override;

    void setParam(juce::String pid);
    void setModId(juce::String mid) override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    bool bipolar;

private:
    TetraOPAudioProcessorEditor& editor;
    float cur_normed_value{ 0.0f };
    juce::Point<int> last_mouse_position;
    juce::Point<int> start_mouse_pos;
    bool mouse_down = false;
    float wheelAccum = 0.f;
};
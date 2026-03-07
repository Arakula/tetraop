#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"
#include "../../engine/Unison.h"

class TetraOPAudioProcessorEditor;
using namespace globals;

class UnisonWidget
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
{
public:
    UnisonWidget(TetraOPAudioProcessorEditor& e, int oscId);
    ~UnisonWidget() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseDoubleClick(const MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    void paint(Graphics& g) override;
    void drawUnisonVoices(Graphics& g);
    void resized() override;

private:
    Rectangle<float> modeBounds{};
    Rectangle<float> voiceBounds{};
    Rectangle<float> spreadBounds{};

    float pixels_per_percent{ 100.0f };
    float cur_normed_value{ 0.0f };
    juce::Point<int> last_mouse_position;
    juce::Point<int> start_mouse_pos;
    bool mouse_down = false;

    bool editingVoices = false;
    bool editingSpread = false;

    String prefix;
    int oscId;
    TetraOPAudioProcessorEditor& editor;
};
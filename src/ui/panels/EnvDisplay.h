#pragma once

#include <JuceHeader.h>
#include "../widgets/Rotary.h"
#include "../../dsp/Envelope.h"

class TetraOPAudioProcessorEditor;

class EnvDisplay
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
    , private juce::Timer
{
public:

    std::unique_ptr<Rotary> delay;
    std::unique_ptr<Rotary> attack;
    std::unique_ptr<Rotary> hold;
    std::unique_ptr<Rotary> decay;
    std::unique_ptr<Rotary> sustain;
    std::unique_ptr<Rotary> release;
    juce::TextButton modeBtn;

    EnvDisplay(TetraOPAudioProcessorEditor& e);
    ~EnvDisplay() override;

    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void connect(juce::String id);
    void disconnect();

    int getSection(const juce::MouseEvent& e);
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    void toggleUIComponents();
    void showEnvelopeModeMenu();

private:
    std::array<juce::Rectangle<int>, 5> envStages;
    juce::String envid = "";
    int envidx = -1;
    TetraOPAudioProcessorEditor& editor;
    int highlighted = -1;
    bool mouse_down = false;
    bool mouse_down_shift = false;
    float cur_normed_value = 0.0f;
    juce::Point<int> last_mouse_position{};
    juce::Point<int> start_mouse_pos{};
    juce::String paramId{};
    bool isActive = false;
};
#pragma once

#include <JuceHeader.h>
#include "../UIUtils.h"
#include "../ModulationDragSource.h"
#include "../../dsp/Pattern.h"
#include "../../dsp/Envelope.h"

class TetraOPAudioProcessorEditor;

class Modulator
    : public juce::Component
    , public ModulationDragSource
    , private juce::AudioProcessorValueTreeState::Listener
    , private juce::Timer
    , public juce::SettableTooltipClient
{
public:
    bool ismodwheel = false;
    bool drawoutline = true;

    Modulator(TetraOPAudioProcessorEditor& e, juce::String modId);
    ~Modulator() override;

    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    juce::Point<float> getDragSource() override;

    void paint(juce::Graphics& g) override;
    void drawValueBuffer(juce::Graphics& g, juce::Rectangle<float>& bounds);
    void drawEnvelope(juce::Graphics& g, juce::Rectangle<float>& bounds);
    void drawLFO(juce::Graphics& g, juce::Rectangle<float>& bounds);
    void resized() override;
    Colour getModColor();

    juce::String modId;
private:
    TetraOPAudioProcessorEditor& editor;
    int modidx = 0;
    float lpad = 20.f;
    bool selected = false;
    uint64_t version = 0; // display id of the modulator, used for LFOs and RNDs
    std::array<float, 30> valbuf{};
    int valbuf_idx = 0;
    bool valbuf_clear = true;
    bool isenv = false;
    bool islfo = false;
    bool ismacro = false;
    bool isrnd = false;
    bool isActive = false;
    int activeCountdown = 0;
    int connections = 0;
};

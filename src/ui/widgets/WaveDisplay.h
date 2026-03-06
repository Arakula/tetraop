#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"
#include "WavetableDisplay.h"

class TetraOPAudioProcessorEditor;
using namespace globals;

class WaveDisplay 
    : public juce::Component 
    , private juce::AudioProcessorValueTreeState::Listener
{
public:

    WaveDisplay(TetraOPAudioProcessorEditor& e, int oscId);
    ~WaveDisplay() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void paint(juce::Graphics& g) override;
    void drawWaveform(juce::Graphics& g);
    void toggleUIComponents();
    void resized() override;

private:
    bool isOn = false;
    String prefix;
    int oscId;
    TetraOPAudioProcessorEditor& editor;
    int mode = 0; // waveform, wavetable, oscilloscope
    WavetableDisplay wtdisplay;
};
#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"
#include "WavetableDisplay.h"
#include "../../engine/PhaseDist.h"
#include "../../engine/TablesManager.h"
#include "../../engine/Utils.h"

class TetraOPAudioProcessorEditor;
using namespace globals;

class WaveDisplay
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
    , private juce::Timer
{
public:
    enum Mode
    {
        Waveform,
        Wavetable,
        Oscilloscope
    };

    WaveDisplay(TetraOPAudioProcessorEditor& e, int oscId);
    ~WaveDisplay() override;
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    void mouseDown(const MouseEvent& e) override;

    void paint(juce::Graphics& g) override;
    void drawWaveform(juce::Graphics& g, float* waveform, int size, float phase, DistFn dist, WindowFn distWindow, float distAmt);
    void toggleUIComponents();
    void resized() override;
    void setMode(Mode _mode);

    bool isMorphing = false; // set from OSCPanel (deprecated)
    bool show3D = false;
private:
    Mode mode = Mode::Oscilloscope;
    bool isOn = false;
    String prefix;
    int oscId;
    TetraOPAudioProcessorEditor& editor;
    WavetableDisplay wtdisplay;
};
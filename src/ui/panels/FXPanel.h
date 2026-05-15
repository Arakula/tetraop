#pragma once

#include <JuceHeader.h>
#include "../UIUtils.h"
#include "../fx/UIFX.h"
#include "../fx/FXCompressor.h"
#include "../fx/FXChorus.h"
#include "../fx/FXDist.h"
#include "../fx/FXDelay.h"
#include "../fx/FXReverb.h"
#include "../fx/FXEQ.h"
#include "../fx/FXPhaser.h"
#include "../../dsp/fx/FX.h"
#include "functional"
#include "../../Globals.h"

using namespace globals;
class TetraOPAudioProcessorEditor;

class FXPanel
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
    , private juce::Timer
{
public:

    FXPanel(TetraOPAudioProcessorEditor& e);
    ~FXPanel() override;

    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void onFXChanged(bool scrollViewport = true);
    void refreshFX(bool scrollViewport = true);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void toggleUIComponents();
    void onDrag(UIFX* fx);
    void onDragEnded(UIFX* fx);

private:
    TetraOPAudioProcessorEditor& editor;

    juce::TextButton layer1Btn;
    juce::TextButton layer2Btn;
    juce::TextButton masterBtn;
    juce::TextButton layer2BypassBtn;
    juce::TextButton layer1BypassBtn;
    juce::TextButton masterBypassBtn;

    juce::OwnedArray<UIFX> fxs;
};

#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"
#include "../UIUtils.h"

class TetraOPAudioProcessor;

class Meter : public juce::Component, private juce::Timer
{
public:
    Meter(TetraOPAudioProcessor& p);
    ~Meter() override;
    void timerCallback() override;

    void paint(juce::Graphics& g) override;

    bool drawLabels = true;
private:
    float rmsSmoothedL = 0.f;
    float rmsSmoothedR = 0.f;
    TetraOPAudioProcessor& audioProcessor;
    float zeroMeter = 0.0f;
};
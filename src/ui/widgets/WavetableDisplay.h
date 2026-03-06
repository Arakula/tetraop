#pragma once

#include "JuceHeader.h"
#include "../../Globals.h"

using namespace globals;

/** Draws a wavetable
 *
 * Adapted from Gin WavetableComponent, slightly modified
*/
class WavetableDisplay : public juce::Component
{
public:
    WavetableDisplay();
    ~WavetableDisplay() override;

    enum ColourIds
    {
        lineColourId             = 0x3331e10,
        backgroundColourId       = 0x3331e11,
        waveColourId             = 0x3331e12,
        activeWaveColourId       = 0x3331f13,
        phaseWaveColourId        = 0x3331f14,
    };

    void showPhase (float start, float len);
    void hidePhase();

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

    void setWavetables (gin::Wavetable*);
    void setParams(gin::WTOscillator::Params params);

    std::function<void (const juce::File&)> onFileDrop;

    enum Style
    {
        a,
        b,
    };

    void setStyle (Style s) { style = s; }

private:
    juce::Path createWavetablePath (float pos, float s = 0.0f, float e = 1.0f)
    {
        if (style == a) return createWavetablePathA (pos, s, e);
        if (style == b) return createWavetablePathB (pos, s, e);
        return {};
    }

    juce::Path createWavetablePathA (float pos, float s = 0.0f, float e = 1.0f);
    juce::Path createWavetablePathB (float pos, float s = 0.0f, float e = 1.0f);

    gin::Wavetable* bllt = nullptr;
    gin::WTOscillator::Params params;
    juce::Array<juce::Path> paths;
    bool needsUpdate = false;
    bool dragOver = false;

    float phaseStart = -1;
    float phaseLen = -1;

    Style style = a;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableDisplay)
};

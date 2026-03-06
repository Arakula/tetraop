#include "WavetableDisplay.h"

WavetableDisplay::WavetableDisplay()
{
}

WavetableDisplay::~WavetableDisplay()
{
}
//==============================================================================

void WavetableDisplay::showPhase (float start, float len)
{
    phaseStart = start;
    phaseLen = len;
    repaint();
}

void WavetableDisplay::hidePhase()
{
    phaseStart = -1;
    phaseLen = -1;
    repaint();
}

void WavetableDisplay::setParams (gin::WTOscillator::Params params_)
{
    if (! juce::approximatelyEqual (params.formant, params_.formant) ||
        ! juce::approximatelyEqual (params.bend, params_.bend)       ||
        ! juce::approximatelyEqual (params.asym, params_.asym)       ||
        ! juce::approximatelyEqual (params.fold, params_.fold) )
    {
        params = params_;
        needsUpdate = true;
        repaint();
    }
    else if (! juce::approximatelyEqual (params.position, params_.position))
    {
        params = params_;
        repaint();
    }
}

void WavetableDisplay::resized()
{
    needsUpdate = true;
    repaint();
}

void WavetableDisplay::setWavetables (gin::Wavetable* bllt_)
{
    bllt = bllt_;
    needsUpdate = true;
    repaint();
}

void WavetableDisplay::paint (juce::Graphics& g)
{
    if (needsUpdate && bllt)
    {
        needsUpdate = false;

        paths.clear();
        auto numTables = std::min (32, bllt->size());
        for (auto i = 0; i < numTables; i++)
            paths.add (createWavetablePath (float (i) / numTables));
    }

    if (paths.size() > 0)
    {
        g.setColour (findColour (waveColourId, true).withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f));
        for (auto& p : paths)
            g.strokePath (p, juce::PathStrokeType (.75f));

        if (isEnabled())
        {
            g.setColour (findColour (activeWaveColourId, true).withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f));
            g.strokePath (createWavetablePath (params.position), juce::PathStrokeType (.75f));
        }
    }
}

juce::Path WavetableDisplay::createWavetablePathB (float wtPos, float start, float end)
{
    constexpr auto samples = 64;

    juce::AudioSampleBuffer buf (2, samples);

    {
        gin::WTOscillator osc;

        auto hz = 44100.0f / samples;
        auto note = gin::getMidiNoteFromHertz (hz);
        auto p = params;

        p.position = wtPos;

        osc.setBlockDC (false);
        osc.setSampleRate (44100.0);
        osc.setWavetable (bllt);
        osc.noteOn (0.0f);
        osc.process (note, p, buf);
    }

    juce::Path p;

    auto w = float (getWidth())  * 0.8f;
    auto h = float (getHeight()) * 0.35f;

    auto x = 10 + (getWidth() - 20 - w) * wtPos;
    auto y = getHeight() / 2.0f + (-(wtPos - 0.5f)) * h;

    auto data = buf.getReadPointer (0);
    auto offset = juce::roundToInt (start * samples);

    p.startNewSubPath (x + w * float (offset) / samples, y + -data[offset] * h);

    for (auto s = 1 + offset; s < std::min (samples, juce::roundToInt (samples * end) + 1); s++)
    {
        p.lineTo (x + w * float (s) / samples, y + -data[s] * h);
    }

    return p;
}

juce::Path WavetableDisplay::createWavetablePathA (float wtPos, float start, float end)
{
    constexpr auto samples = 64;

    juce::AudioSampleBuffer buf (2, samples);

    {
        gin::WTOscillator osc;

        auto hz = 44100.0f / samples;
        auto note = gin::getMidiNoteFromHertz (hz);
        auto p = params;

        p.position = wtPos;

        osc.setSampleRate (44100.0);
        osc.setWavetable (bllt);
        osc.noteOn (0.0f);
        osc.process (note, p, buf);
    }

    juce::Path p;

    auto w = float (getWidth());
    auto h = float (getHeight());

    auto data = buf.getReadPointer (0);

    auto xSpread = 0.4f;
    auto yScale = -(1.0f / 4.5f);
    auto dx = std::min (w, h);

    auto xSlope = (dx * 1.5f * (1.0f - xSpread)) / float (samples);
    auto ySlope = xSlope / 4.0f;

    auto xOffset = (dx * xSpread) * wtPos;
    auto yOffset = (dx * xSpread) - xOffset;

    xOffset += (w - dx * xSpread - samples * xSlope) / 2.0f;
    yOffset += (h - dx * xSpread - samples * ySlope) / 2.0f;

    auto offset = juce::roundToInt (start * samples);
    xOffset += xSlope * offset;
    yOffset += ySlope * offset;

    p.startNewSubPath (xOffset, data[offset] * yScale * dx + yOffset);

    for (auto s = 1 + offset; s < std::min (samples, juce::roundToInt (samples * end) + 1); s++)
    {
        p.lineTo (xOffset, data[s] * yScale * dx + yOffset);

        xOffset += xSlope;
        yOffset += ySlope;
    }

    return p;
}


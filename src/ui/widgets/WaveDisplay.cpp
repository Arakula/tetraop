#include "WaveDisplay.h"
#include "../../PluginEditor.h"

WaveDisplay::WaveDisplay(TetraOPAudioProcessorEditor& e, int _oscId)
    : editor(e)
    , oscId(_oscId)
    , prefix(_oscId == 0 ? "a_" : _oscId == 1 ? "b_" : _oscId == 2 ? "c_" : "d_")
{
    editor.audioProcessor.params.addParameterListener(prefix + "on", this);
    editor.audioProcessor.params.addParameterListener(prefix + "morph", this);
    isOn = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "on")->load();

    addAndMakeVisible(wtdisplay);
    wtdisplay.setWavetables(&editor.audioProcessor.wavetables[oscId].tables);
    wtdisplay.setColour(WavetableDisplay::ColourIds::activeWaveColourId, COLOR_ACTIVE());
    wtdisplay.setColour(WavetableDisplay::ColourIds::waveColourId, COLOR_ACTIVE().darker(0.5f).withAlpha(0.5f));
    wtdisplay.setColour(WavetableDisplay::ColourIds::lineColourId, Colours::transparentBlack);
    wtdisplay.setColour(WavetableDisplay::ColourIds::phaseWaveColourId, Colours::red);
    wtdisplay.setColour(WavetableDisplay::ColourIds::backgroundColourId, Colours::transparentBlack);

    gin::WTOscillator::Params p;
    auto morph = editor.audioProcessor.params.getRawParameterValue(prefix + "morph")->load();
    p.position = morph;
    wtdisplay.setParams(p);
}

WaveDisplay::~WaveDisplay()
{
    editor.audioProcessor.params.removeParameterListener(prefix + "on", this);
    editor.audioProcessor.params.removeParameterListener(prefix + "morph", this);
}

void WaveDisplay::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == prefix + "on")
        isOn = (bool)newValue;

    if (parameterID == prefix + "morph")
    {
        gin::WTOscillator::Params p;
        auto morph = editor.audioProcessor.params.getRawParameterValue(prefix + "morph")->load();
        p.position = morph;
        wtdisplay.setParams(p);

        //mode = 1;
        //toggleUIComponents();
    }

    juce::MessageManager::callAsync([this] { repaint(); });
}

void WaveDisplay::paint(Graphics& g)
{
    if (!isOn) return;

    //drawWaveform(g);
}

void WaveDisplay::resized()
{
    wtdisplay.setBounds(getLocalBounds());
}

void WaveDisplay::toggleUIComponents()
{

}

void WaveDisplay::drawWaveform(Graphics& g)
{
    auto morph = editor.audioProcessor.params.getRawParameterValue(prefix + "morph")->load();
    auto& tables = editor.audioProcessor.wavetables[oscId];
    int tablesz = tables.tableSize;
    auto& table = tables.tables.getTable((int)std::round(((tables.numTables - 1) * morph)))->tableForNote(0.5f);

    auto b = getLocalBounds().toFloat().reduced(4.f, 8.f);

    int width = (int)b.getWidth();
    float centerY = b.getCentreY();
    float scaleY = 0.5f * b.getHeight();

    juce::Path p;
    juce::Array<float> minVals, maxVals;
    minVals.resize(width);
    maxVals.resize(width);

    // compute min/max per pixel column
    for (int x = 0; x < width; ++x)
    {
        int start = (x * tablesz) / width;
        int end = ((x + 1) * tablesz) / width;

        float minVal = 1.0f;
        float maxVal = -1.0f;

        for (int i = start; i < end; ++i)
        {
            float s = table[i];
            minVal = juce::jmin(minVal, s);
            maxVal = juce::jmax(maxVal, s);
        }

        minVals.set(x, minVal);
        maxVals.set(x, maxVal);
    }

    // build top edge (max)
    for (int x = 0; x < width; ++x)
    {
        float px = b.getX() + (float)x;
        float py = centerY - maxVals[x] * scaleY;

        if (x == 0)
            p.startNewSubPath(px, py);
        else
            p.lineTo(px, py);
    }

    // build bottom edge (min) in reverse
    for (int x = width - 1; x >= 0; --x)
    {
        float px = b.getX() + (float)x;
        float py = centerY - minVals[x] * scaleY;
        p.lineTo(px, py);
    }

    p.closeSubPath();

    g.setColour(COLOR_ACTIVE());
    g.strokePath(p, juce::PathStrokeType(1.0f));
}
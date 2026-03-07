#include "WaveDisplay.h"
#include "../../PluginEditor.h"

WaveDisplay::WaveDisplay(TetraOPAudioProcessorEditor& e, int _oscId)
    : editor(e)
    , oscId(_oscId)
    , prefix(_oscId == 0 ? "a_" : _oscId == 1 ? "b_" : _oscId == 2 ? "c_" : "d_")
{
    editor.audioProcessor.params.addParameterListener(prefix + "on", this);
    editor.audioProcessor.params.addParameterListener(prefix + "morph", this);
    editor.audioProcessor.params.addParameterListener(prefix + "phase_offset", this);
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
    wtdisplay.setStyle(WavetableDisplay::b);
    wtdisplay.setInterceptsMouseClicks(false, false);

    startTimerHz(60);
   
    toggleUIComponents();
}

WaveDisplay::~WaveDisplay()
{
    editor.audioProcessor.params.removeParameterListener(prefix + "on", this);
    editor.audioProcessor.params.removeParameterListener(prefix + "morph", this);
    editor.audioProcessor.params.removeParameterListener(prefix + "phase_offset", this);
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
    }

    juce::MessageManager::callAsync([this] { toggleUIComponents(); });
}

void WaveDisplay::timerCallback()
{
    if (isOn && editor.audioProcessor.modulation->lastVoiceIsActive)
    {
        mode = Oscilloscope;
        repaint();
    }
    else if (isMorphing)
    {
        if (mode != Wavetable)
        {
            mode = Wavetable;
            toggleUIComponents();
        }
    }
    else
    {
        if (mode != Waveform)
        {
            mode = Waveform;
            toggleUIComponents();
        }
    }
}

void WaveDisplay::paint(Graphics& g)
{
    if (!isOn || mode == Wavetable) return;

    if (mode == Waveform) {
        auto morph = editor.audioProcessor.params.getRawParameterValue(prefix + "morph")->load();
        auto& tables = editor.audioProcessor.wavetables[oscId];
        int tablesz = tables.tableSize;
        auto& table = tables.tables.getTable((int)std::round(((tables.numTables - 1) * morph)))->tableForNote(0.5f);
        auto phase = editor.audioProcessor.params.getRawParameterValue(prefix + "phase_offset")->load();

        drawWaveform(g, table.data(), tablesz, phase);
    }
    else
    {
        auto waveform = editor.audioProcessor.synth->fm->oscOut[oscId];
        float maxAbs = 0.0f;
        for (int i = 0; i < SCOPE_BUFLEN; ++i)
            maxAbs = std::max(maxAbs, std::abs(waveform[i]));
        float gain = (maxAbs > 1.0f) ? (1.0f / maxAbs) : 1.0f;
        if (gain < 1.f)
            for (int i = 0; i < SCOPE_BUFLEN; ++i)
                waveform[i] *= gain;  // normalize waveform if it exceeds 1

        drawWaveform(g, waveform.data(), SCOPE_BUFLEN, 0);
    }
}

void WaveDisplay::resized()
{
    wtdisplay.setBounds(getLocalBounds());
}

void WaveDisplay::toggleUIComponents()
{
    wtdisplay.setVisible(mode == 1 && isOn);
    repaint();
}

void WaveDisplay::setMode(Mode _mode)
{
    mode = _mode;
    toggleUIComponents();
}

void WaveDisplay::drawWaveform(Graphics& g, float* waveform, int size, float phase)
{
    auto b = getLocalBounds().toFloat().reduced(4.f, 8.f);

    int width = (int)b.getWidth();
    float centerY = b.getCentreY();
    float scaleY = 0.5f * b.getHeight();
    int phaseOffset = (int)(phase * size);

    juce::Path p;

    // start at first sample
    if (size > 0) {
        int index = phaseOffset % size;
        p.startNewSubPath(b.getX(), centerY - waveform[index] * scaleY);
    }

    // map each sample to pixel x
    for (int x = 1; x < width; ++x)
    {
        // pick corresponding sample in waveform
        int index = (x * size) / width;
        index = (index + phaseOffset) % size;

        float px = b.getX() + (float)x;
        float py = centerY - waveform[index] * scaleY;

        p.lineTo(px, py);
    }

    g.setColour(COLOR_ACTIVE());
    g.strokePath(p, juce::PathStrokeType(1.5f));
}
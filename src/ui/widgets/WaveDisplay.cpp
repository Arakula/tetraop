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
    editor.audioProcessor.params.addParameterListener(prefix + "phase_dist_amt", this);
    editor.audioProcessor.params.addParameterListener(prefix + "phase_dist_mode", this);
    isOn = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "on")->load();

    addAndMakeVisible(wtdisplay);
    wtdisplay.setWavetables(&editor.audioProcessor.tablesMgr->wavetables[oscId].tables);
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
    editor.audioProcessor.params.removeParameterListener(prefix + "phase_dist_amt", this);
    editor.audioProcessor.params.removeParameterListener(prefix + "phase_dist_mode", this);
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
        if (mode != Oscilloscope)
        {
            mode = Oscilloscope;
            toggleUIComponents();
        }
        repaint();
    }
    else if (show3D && mode != Wavetable)
    {
        mode = Wavetable;
        toggleUIComponents();
    }
    else if (!show3D && mode != Waveform)
    {
        mode = Waveform;
        toggleUIComponents();
    }
}

void WaveDisplay::mouseDown(const MouseEvent&)
{
    show3D = !show3D;
}

void WaveDisplay::paint(Graphics& g)
{
    if (!isOn || mode == Wavetable) return;

    if (mode == Waveform) {
        auto morph = editor.audioProcessor.params.getRawParameterValue(prefix + "morph")->load();
        auto morphSnap = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "morph_snap")->load();
        auto& tables = editor.audioProcessor.tablesMgr->wavetables[oscId];
        auto phase = editor.audioProcessor.params.getRawParameterValue(prefix + "phase_offset")->load();
        auto distAmt = editor.audioProcessor.params.getRawParameterValue(prefix + "phase_dist_amt")->load();
        auto distMode = (PhaseDist::Mode)editor.audioProcessor.params.getRawParameterValue(prefix + "phase_dist_mode")->load();

        int tablesz = tables.tableSize;
        int ntables = tables.numTables;
        auto idx = std::min(ntables - 1, int(float(ntables) * morph));
        std::vector<float> table;

        if (tables.mode == TablesManager::PinkNoise)
        {
            distMode = PhaseDist::Off;
            PinkNoiseGen gen(1000);
            tablesz = 1024;
            table.reserve(tablesz);
            for (int i = 0; i < tablesz; ++i)
                table.push_back(gen.next());
        }
        else if (tables.mode == TablesManager::WhiteNoise)
        {
            distMode = PhaseDist::Off;
            WhiteNoiseGen gen(1000);
            tablesz = 1024;
            table.reserve(tablesz);
            for (int i = 0; i < tablesz; ++i)
                table.push_back(gen.next());
        }
        else
        {
            table = tables.tables.getTable(idx)->tableForNote(0.f);
            if (!morphSnap && idx < tables.numTables - 1)
            {
                auto& t2 = tables.tables.getTable(idx + 1)->tableForNote(0.f);
                float pos = float(ntables) * morph;
                float frac = pos - std::floor(pos);
                for (int i = 0; i < tablesz; ++i)
                {
                    table[i] = table[i] * (1 - frac) + t2[i] * frac;
                }
            }
        }

        std::array<DistFn, 9> dists = {
            PhaseDist::bypass,
            PhaseDist::bend,
            PhaseDist::skew,
            PhaseDist::bias,
            PhaseDist::pulse,
            PhaseDist::sync,
            PhaseDist::sync,
            PhaseDist::quantize,
            PhaseDist::fold
        };

        drawWaveform(g, table.data(), tablesz, phase, 
            distMode > 0 ? dists[distMode] : nullptr, 
            distMode == PhaseDist::Formant ? PhaseDist::windowHalfSine : nullptr, 
            distAmt
        );
    }
    else // oscilloscope
    {
        auto level = editor.audioProcessor.params.getRawParameterValue(prefix + "level")->load();
        if (level > 0.f) {
            auto waveform = editor.audioProcessor.synth->fm->oscOut[oscId];
            float maxAbs = 0.0f;
            for (int i = 0; i < SCOPE_BUFLEN; ++i)
                maxAbs = std::max(maxAbs, std::abs(waveform[i]));
            float gain = (maxAbs > 1.0f) ? (1.0f / maxAbs) : 1.0f;
            if (gain < 1.f)
                for (int i = 0; i < SCOPE_BUFLEN; ++i)
                    waveform[i] *= gain;  // normalize waveform if it exceeds 1
            drawWaveform(g, waveform.data(), SCOPE_BUFLEN, 0, nullptr, nullptr, 0.f);
        }
        else
        {
            std::array<float, 2> empty{};
            drawWaveform(g, empty.data(), 2, 0, nullptr, nullptr, 0.f);
        }
    }

    if (dragOver)
    {
        g.setColour(Colours::green.withAlpha(.25f));
        g.fillRect(getLocalBounds().toFloat().reduced(2.f));
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

void WaveDisplay::drawWaveform(Graphics& g, float* waveform, int size, float phase, DistFn dist, WindowFn distWindow, float distAmt)
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
        float phs = (float)x / width + phase;
        float phsorig = phs;
        if (phs > 1.f) phs -= 1.f;

        if (dist)
            phs = dist(SIMDF(phs), distAmt).getfirst();

        int index = (int)(phs * (size - 1));

        float px = b.getX() + (float)x;
        auto val = waveform[index];

        if (distWindow) {
            auto simdval = SIMDF(val);
            auto simdphs = SIMDF(phsorig);
            distWindow(simdval, simdphs);
            val = simdval.getfirst();
        }

        float py = centerY - val * scaleY;

        p.lineTo(px, py);
    }

    g.setColour(COLOR_ACTIVE());
    g.strokePath(p, juce::PathStrokeType(1.5f));
}

bool WaveDisplay::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (files.size() == 1 && (juce::File(files[0]).hasFileExtension(".wav") || juce::File(files[0]).hasFileExtension(".flac")))
        return true;

    return false;
}

void WaveDisplay::fileDragEnter(const juce::StringArray&, int, int)
{
    dragOver = true;
    repaint();
}

void WaveDisplay::fileDragExit(const juce::StringArray&)
{
    dragOver = false;
    repaint();
}

void WaveDisplay::filesDropped(const juce::StringArray& files, int, int)
{
    dragOver = false;
    repaint();

    File file(files[0]);
    MemoryBlock block;

    if (file.loadFileAsData(block))
    {
        auto b64 = juce::Base64::toBase64(block.getData(), block.getSize());
        editor.audioProcessor.tablesMgr->loadUserTable(oscId, file.getFileName(), b64);
    }
}
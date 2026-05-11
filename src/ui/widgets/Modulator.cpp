#include "Modulator.h"
#include "../../PluginEditor.h"

Modulator::Modulator(TetraOPAudioProcessorEditor& e, juce::String modId)
    : modId(modId)
    , editor(e)
{
    startTimerHz(30);
    isenv = modId.startsWith("env");
    islfo = modId.startsWith("lfo");
    ismacro = modId.startsWith("macro");
    isrnd = modId.startsWith("rnd");
    modidx = modId.retainCharacters("0123456789").getIntValue() - 1;

    if (isenv) {
        editor.audioProcessor.params.addParameterListener(modId + "_del", this);
        editor.audioProcessor.params.addParameterListener(modId + "_att", this);
        editor.audioProcessor.params.addParameterListener(modId + "_hld", this);
        editor.audioProcessor.params.addParameterListener(modId + "_dec", this);
        editor.audioProcessor.params.addParameterListener(modId + "_sus", this);
        editor.audioProcessor.params.addParameterListener(modId + "_rel", this);
        editor.audioProcessor.params.addParameterListener(modId + "_tenatt", this);
        editor.audioProcessor.params.addParameterListener(modId + "_tendec", this);
        editor.audioProcessor.params.addParameterListener(modId + "_tenrel", this);
    }
}

Modulator::~Modulator()
{
    if (isenv) {
        editor.audioProcessor.params.removeParameterListener(modId + "_del", this);
        editor.audioProcessor.params.removeParameterListener(modId + "_att", this);
        editor.audioProcessor.params.removeParameterListener(modId + "_hld", this);
        editor.audioProcessor.params.removeParameterListener(modId + "_dec", this);
        editor.audioProcessor.params.removeParameterListener(modId + "_sus", this);
        editor.audioProcessor.params.removeParameterListener(modId + "_rel", this);
        editor.audioProcessor.params.removeParameterListener(modId + "_tenatt", this);
        editor.audioProcessor.params.removeParameterListener(modId + "_tendec", this);
        editor.audioProcessor.params.removeParameterListener(modId + "_tenrel", this);
    }
}

/*
* Modulators use polling to update state and repaint
* not the best solution but keeps the code self contained
*/
void Modulator::timerCallback()
{
    if (!isVisible()) return;
    auto selectedModId = juce::String(editor.audioProcessor.modulation->selectedMod);
    Modulation::Modulator mod = editor.audioProcessor.modulation->modulators[modId];

    bool env1Active = modId == "env1";

    if (mod.active && (mod.connections || env1Active)) {
        valbuf_clear = false;
        valbuf[valbuf_idx] = mod.value;
        valbuf_idx = (valbuf_idx + 1) % valbuf.size();
        isActive = true;
        activeCountdown = 30; // 30 hz
        repaint();
    }
    // keep display active for one second
    else if (isActive) {
        activeCountdown--;
        if (activeCountdown == 0 || islfo) {
            isActive = false;
        }
        valbuf[valbuf_idx] = 0.0f;
        valbuf_idx = (valbuf_idx + 1) % (int)valbuf.size();
        repaint();
    }
    else if (valbuf_clear == false) {
        std::fill(valbuf.begin(), valbuf.end(), 0.f);
        valbuf_clear = true;
        repaint();
    }

    if (islfo) {
        auto& lfo = editor.audioProcessor.modulation->lfos[modidx];
        if (version != lfo.pattern.versionID) {
            version = lfo.pattern.versionID;
            repaint();
        }
    }

    if (selected && selectedModId != modId) {
        selected = false;
        repaint();
    }
    else if (!selected && selectedModId == modId) {
        selected = true;
        repaint();
    }

    if (connections != mod.connections) {
        connections = mod.connections;
        repaint();
    }
}

void Modulator::parameterChanged(const juce::String& parameterID, float newValue)
{
    (void)parameterID;
    (void)newValue;
    juce::MessageManager::callAsync([this]() { repaint(); });
}

void Modulator::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) {
        //editor.showModContextMenu(this);
    }
}

void Modulator::mouseUp(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) return;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    if (editor.audioProcessor.modulation->selectedMod != modId) {
        editor.audioProcessor.modulation->setSelectedMod(modId);
    }
}

void Modulator::mouseDrag(const juce::MouseEvent& e)
{
    if (!editor.isDragDropModulation && !getLocalBounds().contains(e.getPosition())) {
        editor.startDragDrop(modId, this);
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }
}

juce::Point<float> Modulator::getDragSource()
{
    // drawHandle is a 14x14 shape; its centre sits at +(7, 7) from the
    // bounds passed into UIUtils::drawHandle in paint().
    constexpr float c = 7.f;
    if (ismodwheel)
        return { 8.f + c, 4.f + c };
    return { 3.f + c, 3.f + c };
}

void Modulator::paint(juce::Graphics& g)
{
    if (!isVisible()) return;
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);

    // draw bg
    g.setColour(Colours::white.withAlpha(0.02f));
    g.fillRoundedRectangle(bounds, 2.f);
    juce::Path p;
    p.addRoundedRectangle(bounds.getX(), bounds.getY(), lpad, bounds.getHeight(), 2.f, 2.f, true, false, true, false);
    g.setColour(Colour(0xff333333));
    g.fillPath(p);
    if (isenv && editor.audioProcessor.displayEnv == modId)
    {
        g.setColour(COLOR_ENVELOPE().withAlpha(0.075f));
        g.fillRoundedRectangle(bounds, 2.f);
    }
    else if (islfo && editor.audioProcessor.displayLfo == modId)
    {
        g.setColour(COLOR_LFO().withAlpha(0.075f));
        g.fillRoundedRectangle(bounds, 2.f);
    }

    // draw outline
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.drawVerticalLine((int)(bounds.getX() + lpad), bounds.getY(), bounds.getBottom());
    g.setColour(juce::Colour(selected
        ? COLOR_ACTIVE()
        : juce::Colours::black.withAlpha(0.25f)));
    g.drawRoundedRectangle(bounds, 2.f, 1.f);

    // draw handle
    UIUtils::drawHandle(g, bounds
        .withTrimmedRight(bounds.getWidth() - lpad)
        .withHeight(bounds.getHeight() / 2.f)
        .translated(3.f, 3.f), juce::Colours::white);

    g.setFont(juce::FontOptions(10.f));
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawFittedText(juce::String(connections), bounds
        .withTrimmedRight(bounds.getWidth() - lpad)
        .withTrimmedTop(bounds.getHeight() / 2.f).toNearestInt(),
        juce::Justification::centred, 1);

    g.setFont(juce::FontOptions(12.f));
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawFittedText(modId.toUpperCase(), bounds
        .withTrimmedLeft(lpad)
        .withTrimmedBottom(bounds.getHeight() / 2.f).toNearestInt()
        , juce::Justification::centred, 1);
    //

    auto drawBounds = bounds
        .withTrimmedTop(bounds.getHeight() / 2)
        .withTrimmedLeft(lpad)
        .reduced(1.f);

    if (ismodwheel) {
        drawBounds = bounds
            .withTrimmedTop(bounds.getHeight() / 2)
            .reduced(1.f);
    }

    auto& mod = editor.audioProcessor.modulation->modulators[modId];
    bool env1Active = modId == "env1";

    if (isActive || (mod.active && (mod.connections || env1Active))) {
        drawValueBuffer(g, drawBounds);
    }
    else if (isenv) {
        drawEnvelope(g, drawBounds);
    }
    else if (islfo) {
        drawLFO(g, drawBounds);
    }
    else if (isrnd) {
        UIUtils::drawRandGen(g, drawBounds.translated(5.f, 1.f), juce::Colours::white.withAlpha(0.5f));
    }
    else {
        if (modId == "mod") {
            UIUtils::drawModwheel(g, drawBounds.translated(12.5f, 0.f), juce::Colours::white.withAlpha(0.5f));
        }
        else if (modId == "vel") {
            UIUtils::drawVel(g, drawBounds.translated(13.f, 0.f), juce::Colours::white.withAlpha(0.5f));
        }
        else if (modId == "key") {
            UIUtils::drawKeys(g, drawBounds.translated(13.5f, 0.5f), juce::Colours::white.withAlpha(0.5f));
        }
        else if (modId == "at") {
            UIUtils::drawHand(g, drawBounds.translated(13.f, 0.f), juce::Colours::white.withAlpha(0.5f));
        }
        else if (modId == "rand") {
            UIUtils::drawRand(g, drawBounds.translated(7.5f, 1.5f), juce::Colours::white.withAlpha(0.5f));
        }
        else if (modId == "x") {
            UIUtils::drawXMod(g, drawBounds.translated(10.f, 3.f), juce::Colours::white.withAlpha(0.5f));
        }
        else if (modId == "y") {
            UIUtils::drawYMod(g, drawBounds.translated(15.f, 0.f), juce::Colours::white.withAlpha(0.5f));
        }
        else if (modId == "z") {
            UIUtils::drawZMod(g, drawBounds.translated(7.f, 3.f), juce::Colours::white.withAlpha(0.5f));
        }
        else if (modId == "lift") {
            UIUtils::drawLift(g, drawBounds.translated(14.f, 0.f), juce::Colours::white.withAlpha(0.5f));
        }
    }
}

void Modulator::drawValueBuffer(juce::Graphics& g, juce::Rectangle<float>& bounds)
{
    juce::Path p;
    auto pixelCount = (int)bounds.getWidth();
    auto bufSize = (int)valbuf.size();
    bool pathOpen = false;
    for (int x = 0; x < pixelCount; ++x)
    {
        float t = (float)x / (pixelCount - 1);
        float pos = t * (bufSize - 1);
        int i0 = (int)pos;
        int i1 = (i0 + 1 < bufSize) ? i0 + 1 : i0;
        float frac = pos - i0;

        int index0 = (valbuf_idx + i0) % bufSize;
        int index1 = (valbuf_idx + i1) % bufSize;

        float v0 = valbuf[index0];
        float v1 = valbuf[index1];

        // If either side is zero, break the path - don't interpolate through zeros
        if ((isenv && v0 == 0.f && v1 == 0.f) ||
            (!isenv && (v0 == 0.0f || v1 == 0.0f)))
        {
            pathOpen = false;
            continue;
        }

        float value = v0 * (1.0f - frac) + v1 * frac;

        float px = bounds.getX() + x;
        float py = bounds.getY() + (1.0f - value) * bounds.getHeight();

        if (!pathOpen) {
            p.startNewSubPath(px, py);
            pathOpen = true;
        }
        else {
            p.lineTo(px, py);
        }
    }
    juce::Colour c = juce::Colour(isenv
        ? COLOR_ENVELOPE()
        : islfo ? COLOR_LFO()
        : ismacro ? COLOR_MACRO()
        : isrnd ? COLOR_RND()
        : COLOR_OTHER_MOD()
    );
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.f));
}

void Modulator::drawEnvelope(juce::Graphics& g, juce::Rectangle<float>& bounds)
{
    auto mode = (Envelope::Mode)editor.audioProcessor.params.getRawParameterValue(modId + "_mode")->load();
    auto del = editor.audioProcessor.params.getRawParameterValue(modId + "_del")->load();
    auto att = std::max(0.0001f, editor.audioProcessor.params.getRawParameterValue(modId + "_att")->load());
    auto hld = editor.audioProcessor.params.getRawParameterValue(modId + "_hld")->load();
    auto dec = std::max(0.0001f, editor.audioProcessor.params.getRawParameterValue(modId + "_dec")->load());
    auto sus = editor.audioProcessor.params.getRawParameterValue(modId + "_sus")->load();
    auto rel = editor.audioProcessor.params.getRawParameterValue(modId + "_rel")->load();
    auto tatt = editor.audioProcessor.params.getRawParameterValue(modId + "_tenatt")->load();
    auto tdec = editor.audioProcessor.params.getRawParameterValue(modId + "_tendec")->load();
    auto trel = editor.audioProcessor.params.getRawParameterValue(modId + "_tenrel")->load();

    if (mode == Envelope::ADSR) {
        del = 0.f;
        hld = 0.f;
    }
    else if (mode == Envelope::AHD) {
        del = 0.f;
        sus = 0.f;
        rel = 0.0001f;
    }
    else if (mode == Envelope::DADSR) {
        hld = 0.f;
    }

    auto total = del + att + hld + dec + rel;

    Pattern p{-1};

    p.insertPoint(0.f, 0.f, tatt, 1);
    p.insertPoint(del / total, 0.f, tatt, 1);
    p.insertPoint((del + att) / total, 1.f, tdec, 1);
    p.insertPoint((del + att + hld) / total, 1.f, tdec, 1);
    p.insertPoint((del + att + hld + dec) / total, sus, trel, 1);
    p.insertPoint(1.f, 0.f, 0.f, 1);
    p.buildSegments();


    juce::Path path;
    auto pixels = bounds.getWidth();
    for (int x = 0; x < pixels; ++x) {
        float t = (float)x / (pixels - 1);
        auto px = bounds.getX() + x;
        auto py = bounds.getY() + bounds.getHeight() * (1- p.get_y_at(t));
        if (x == 0)
            path.startNewSubPath(px, py);
        else
            path.lineTo(px, py);
    }

    g.setColour(COLOR_ENVELOPE());
    g.strokePath(path, juce::PathStrokeType(1.f));
}

void Modulator::drawLFO(juce::Graphics& g, juce::Rectangle<float>& bounds)
{
    auto& lfo = editor.audioProcessor.modulation->lfos[modidx];

    juce::Path path;
    auto pixels = bounds.getWidth();
    for (int x = 0; x < pixels; ++x) {
        float t = (float)x / (pixels - 1);
        auto px = bounds.getX() + x;
        auto py = bounds.getY() + bounds.getHeight() * (1 - lfo.pattern.get_y_at(t));
        if (x == 0)
            path.startNewSubPath(px, py);
        else
            path.lineTo(px, py);
    }

    g.setColour(juce::Colour(COLOR_LFO()));
    g.strokePath(path, juce::PathStrokeType(1.f));
}

void Modulator::resized()
{
}

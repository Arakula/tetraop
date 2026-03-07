#include "UnisonWidget.h"
#include "../../PluginEditor.h"

UnisonWidget::UnisonWidget(TetraOPAudioProcessorEditor& e, int _oscId)
    : editor(e)
    , oscId(_oscId)
    , prefix(_oscId == 0 ? "a_" : _oscId == 1 ? "b_" : _oscId == 2 ? "c_" : "d_")
{
    editor.audioProcessor.params.addParameterListener(prefix + "unison_mode", this);
    editor.audioProcessor.params.addParameterListener(prefix + "unison_voices", this);
    editor.audioProcessor.params.addParameterListener(prefix + "unison_detune", this);
    editor.audioProcessor.params.addParameterListener(prefix + "unison_spread", this);
    editor.audioProcessor.params.addParameterListener(prefix + "unison_blend", this);
}

UnisonWidget::~UnisonWidget()
{
    editor.audioProcessor.params.removeParameterListener(prefix + "unison_mode", this);
    editor.audioProcessor.params.removeParameterListener(prefix + "unison_voices", this);
    editor.audioProcessor.params.removeParameterListener(prefix + "unison_detune", this);
    editor.audioProcessor.params.removeParameterListener(prefix + "unison_spread", this);
    editor.audioProcessor.params.removeParameterListener(prefix + "unison_blend", this);
}

void UnisonWidget::parameterChanged(const juce::String& parameterID, float newValue)
{
    (void)parameterID;
    (void)newValue;

    juce::MessageManager::callAsync([this] { repaint(); });
}

void UnisonWidget::mouseDown(const MouseEvent& e)
{
    if (mouse_down) return;
    mouse_down = true;

    editingVoices = voiceBounds.contains(e.getPosition().toFloat());
    editingSpread = spreadBounds.contains(e.getPosition().toFloat());

    if (!editingVoices && !editingSpread)
        return;

    e.source.enableUnboundedMouseMovement(true);
    mouse_down = true;
    auto param = editor.audioProcessor.params.getParameter(editingVoices ? prefix + "unison_voices" : prefix + "unison_spread");
    auto cur_val = param->getValue();
    cur_normed_value = cur_val;
    last_mouse_position = e.getPosition();
    setMouseCursor(MouseCursor::NoCursor);
    start_mouse_pos = Desktop::getInstance().getMousePosition();
    repaint();
    param->beginChangeGesture();

    if (editingSpread)
    {
        if (!spreadValuePopup) {
            spreadValuePopup = std::make_unique<juce::Label>();
            spreadValuePopup->setJustificationType(juce::Justification::centred);
            spreadValuePopup->setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.8f));
            spreadValuePopup->setColour(juce::Label::textColourId, juce::Colours::white);
            spreadValuePopup->setBorderSize(juce::BorderSize<int>(2));
            spreadValuePopup->setSize(60, 40);
        }

        auto* top = getTopLevelComponent();
        auto target = top->getLocalArea(this, spreadBounds.translated(0, 30));
        spreadValuePopup->setTopLeftPosition((int)(target.getCentreX() - spreadValuePopup->getWidth() / 2),
            (int)(target.getY() + target.getHeight() / 2 - spreadValuePopup->getHeight() / 2));
        spreadValuePopup->setText(String("Warp\n") + String(std::round(param->getValue() * 200.f - 100.f)) + "%", dontSendNotification);
        if (spreadValuePopup->getParentComponent() != top)
            top->addAndMakeVisible(*spreadValuePopup);
    }
}

void UnisonWidget::positionSpreadValuePopup()
{
    auto* top = getTopLevelComponent();
    
}

void UnisonWidget::mouseUp(const MouseEvent& e)
{
    if (!mouse_down)
        return;

    spreadValuePopup.reset();
    mouse_down = false;
    setMouseCursor(MouseCursor::NormalCursor);
    e.source.enableUnboundedMouseMovement(false);
    Desktop::getInstance().setMousePosition(start_mouse_pos);
    repaint();

    auto param = editor.audioProcessor.params.getParameter(editingVoices ? prefix + "unison_voices" : prefix + "unison_spread");
    param->endChangeGesture();
    editingVoices = false;
    editingSpread = false;
}

void UnisonWidget::mouseDrag(const MouseEvent& e)
{
    if (!mouse_down) return;
    auto change = e.getPosition() - last_mouse_position;
    last_mouse_position = e.getPosition();
    auto speed = (e.mods.isShiftDown() ? 40.0f : 4.0f) * pixels_per_percent;
    auto slider_change = float(-change.getY()) / speed;
    cur_normed_value += slider_change;
    auto param = editor.audioProcessor.params.getParameter(editingVoices ? prefix + "unison_voices" : prefix + "unison_spread");
    param->setValueNotifyingHost(cur_normed_value);

    if (editingSpread)
    {
        spreadValuePopup->setText(String("Warp\n") + String(std::round(param->getValue() * 200.f - 100.f)) + "%", dontSendNotification);
    }
}

void UnisonWidget::mouseDoubleClick(const MouseEvent& e)
{
    if (voiceBounds.contains(e.getPosition().toFloat()))
    {
        auto param = editor.audioProcessor.params.getParameter(prefix + "unison_voices");
        param->setValueNotifyingHost(param->getDefaultValue());
    }
    else if (spreadBounds.contains(e.getPosition().toFloat()))
    {
        auto param = editor.audioProcessor.params.getParameter(prefix + "unison_spread");
        param->setValueNotifyingHost(param->getDefaultValue());
    }
}

void UnisonWidget::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (mouse_down) return; // prevent crash, param is already mutating

    bool _editingVoices = voiceBounds.contains(e.getPosition().toFloat());
    bool _editingSpread = spreadBounds.contains(e.getPosition().toFloat());

    if (!_editingVoices && !_editingSpread) 
        return;

    auto speed = (e.mods.isShiftDown() ? 0.01f : 0.05f);
    auto slider_change = wheel.deltaY > 0 ? speed : wheel.deltaY < 0 ? -speed : 0;

    auto param = editor.audioProcessor.params.getParameter(_editingVoices ? prefix + "unison_voices" : prefix + "unison_spread");
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->getValue() + slider_change);
    while (wheel.deltaY > 0.0f && param->getValue() == 0.0f) { // FIX wheel not working when value is zero, first step takes more than 0.05%
        slider_change += 0.05f;
        param->setValueNotifyingHost(param->getValue() + slider_change);
    }
    param->endChangeGesture();
}

void UnisonWidget::paint(Graphics& g)
{
    int voices = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "unison_voices")->load();
    auto mode = (Unison::Mode)editor.audioProcessor.params.getRawParameterValue(prefix + "unison_mode")->load();
    auto modeStr = StringArray{"Unison", "Gauss", "Alt", "5ths", "Sub"}[mode];

    g.setColour(COLOR_VIEWPORT_TEXT());
    g.setFont(FontOptions(16.f));
    g.drawText(modeStr, modeBounds, Justification::centred);
    g.drawText(String(voices), voiceBounds, Justification::centred);

    g.setColour(Colours::white.withAlpha(0.20f));
    g.drawVerticalLine((int)voiceBounds.getX(), voiceBounds.getY() + 2, voiceBounds.getBottom() - 2);
    g.drawVerticalLine((int)voiceBounds.getRight(), voiceBounds.getY() + 2, voiceBounds.getBottom() - 2);
    g.drawHorizontalLine((int)modeBounds.getY(), modeBounds.getX() + 2, spreadBounds.getRight() - 2);

    drawUnisonVoices(g);
}

void UnisonWidget::drawUnisonVoices(Graphics& g)
{
    int voices = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "unison_voices")->load();
    float detune = editor.audioProcessor.params.getRawParameterValue(prefix + "unison_detune")->load();
    float blend = editor.audioProcessor.params.getRawParameterValue(prefix + "unison_blend")->load();
    float spread = editor.audioProcessor.params.getRawParameterValue(prefix + "unison_spread")->load();
    auto b = spreadBounds.reduced(2.f);
    auto uni = Unison::generateDetuneRatios(voices, detune, spread);
    auto gain = Unison::generateVoicesGain(voices, blend, false);

    auto ratioToNormal = [this](float ratio)
        {
            if (ratio == 0) return 0.5f;
            float semitones = 12.0f * std::log2(ratio);   // convert ratio -> semitones
            return (semitones + 2.0f) / 4.0f;            // map -2..2 semitones to normal pos
        };

    g.setColour(COLOR_ACTIVE().withAlpha(0.7f));
    for (int i = 0; i < voices; ++i)
    {
        auto pos = ratioToNormal(uni[i]);
        auto x = (int)std::round(b.getX() + pos * b.getWidth());
        
        g.drawVerticalLine(x, b.getBottom() - b.getHeight() * gain[i], b.getBottom());
    }
}

void UnisonWidget::resized()
{
    auto b = getLocalBounds().toFloat();
    auto left = b.getCentreX() - 15;
    voiceBounds = Rectangle<float>(left, b.getY(), 30, b.getHeight());
    modeBounds = Rectangle<float>(b.getX(), b.getY(), left, b.getHeight());
    spreadBounds = Rectangle<float>(voiceBounds.getRight(), b.getY(), b.getWidth() - voiceBounds.getRight(), b.getHeight());
}
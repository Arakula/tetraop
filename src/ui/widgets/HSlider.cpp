#include "HSlider.h"
#include "../../PluginEditor.h"
#include "../../Globals.h"

HSlider::HSlider(TetraOPAudioProcessorEditor& e, juce::String _paramId, juce::String name, Format format, bool isSymmetric, juce::Colour _trackColor)
    : ModulatedParam(_paramId)
    , format(format)
    , name(name)
    , editor(e)
    , isSymmetric(isSymmetric)
    , trackColor(_trackColor)
{
    setTitle(name);
    setWantsKeyboardFocus(true);
    editor.audioProcessor.params.addParameterListener(paramId, this);
}

HSlider::~HSlider()
{
    if (mouse_down) {
        auto param = editor.audioProcessor.params.getParameter(editingMod ? modId : paramId);
        param->endChangeGesture();
        mouse_down = false;
    }
    editor.audioProcessor.params.removeParameterListener(paramId, this);
    if (modId.isNotEmpty()) {
        editor.audioProcessor.params.removeParameterListener(modId, this);
    }
}

void HSlider::parameterChanged(const juce::String& parameterID, float newValue)
{
    (void)parameterID;
    (void)newValue;
    juce::MessageManager::callAsync([this] { repaint(); });
}

void HSlider::setParamId(juce::String pid)
{
    editor.audioProcessor.params.removeParameterListener(paramId, this);
    paramId = pid;
    editor.audioProcessor.params.addParameterListener(paramId, this);
    setName(paramId);
    repaint();
}

void HSlider::setModId(juce::String mid)
{
    if (modId.isNotEmpty()) {
        editor.audioProcessor.params.removeParameterListener(modId, this);
    }
    modId = mid;
    if (modId.isNotEmpty()) {
        editor.audioProcessor.params.addParameterListener(modId, this);
    }
    repaint();
}

void HSlider::paint(juce::Graphics& g) {
    auto param = editor.audioProcessor.params.getParameter(paramId);
    float normValue = mouse_down && !editingMod
        ? cur_normed_value
        : param->getValue();

	auto bounds = getLocalBounds().toFloat();
    bool disabled = !isEnabled();

    g.setColour(Colour(0xff333333));
    g.drawHorizontalLine(4, bounds.getX(), bounds.getRight());

    auto trackArea = bounds.withTrimmedTop(4.f);
    float trackInset = 11.f;
    track = trackArea.reduced(trackInset, 4.f);

    // draw track
    g.setColour(COLOR_BACKGROUND());
    g.fillRoundedRectangle(track, 2.f);

	float fillWidth = track.getWidth() * normValue;
    g.setColour(trackColor);
    if (disabled) g.setOpacity(0.3f);

    if (isSymmetric) {
        float centerX = track.getCentreX();
        float offset = (normValue - 0.5f) * track.getWidth();

        juce::Rectangle<float> fillRect = offset >= 0.0f
            ? juce::Rectangle<float>(centerX,track.getY() + 1.f,offset, 2.f)
            : juce::Rectangle<float>(centerX + offset, track.getY() + 1.f, -offset, 2.f);

        g.fillRoundedRectangle(fillRect, 2.f);
    }
    else {
        juce::Rectangle<float> fillRect(track.getX(), track.getY() + 1.f, track.getWidth() * normValue, 2.f);
        g.fillRoundedRectangle(fillRect, 2.f);
    }

    // modulation lane
    auto modLane = bounds.withHeight(4.f).reduced(trackInset, 1.f);
    float handleW = 22.f;
    float handleH = 12.f;

    if (!disabled && modId.isNotEmpty()) {
        float amount = editor.audioProcessor.params.getRawParameterValue(modId)->load();
        if (modBipolar) {
            amount *= 0.5f;
        }
        float handleX = modLane.getX() + (modLane.getWidth() * normValue);
        float modOffset = amount * modLane.getWidth();
        float modX = std::clamp(handleX + modOffset, modLane.getX(), modLane.getX() + modLane.getWidth());

        float modRight = std::fmin(handleX, modX);
        float modWidth = std::fabs(modX - handleX);

        g.setColour(modColor);
        g.fillRect(modRight, modLane.getY(), modWidth, modLane.getHeight());
        if (modBipolar) {
            g.setColour(modColor.withMultipliedAlpha(0.5f));
            float inverseX = std::clamp(handleX - modOffset,
                modLane.getX(),
                modLane.getX() + modLane.getWidth());

            float invRight = std::min(handleX, inverseX);
            float invWidth = std::fabs(inverseX - handleX);

            g.fillRect(invRight, modLane.getY(), invWidth, modLane.getHeight());
        }
    }

    // draw modvalue
    if (!disabled && modulated) {
        const float value = voiceActive ? modValue : normValue;
        auto r = KNOB_MODVAL_RADIUS;
        float dotX = modLane.getX() + modLane.getWidth() * value;
        float dotY = modLane.getY() + modLane.getHeight() * 0.5f;

        // Draw small circle radius 1.0f
        g.setColour(COLOR_PANEL());
        g.fillEllipse(dotX - r * 2, dotY - r * 2, r * 4, r * 4);
        g.setColour(trackColor);
        g.fillEllipse(dotX - r, dotY - r, r * 2, r * 2);
    }

    // draw handle

    ///////////////////////////////
    // handle
    float capAlpha = disabled ? 0.3f : 1.0f;
    handle = juce::Rectangle<float>(handleW, handleH).translated(track.getX() + fillWidth - handleW * 0.5f, track.getCentreY() - handleH * 0.5f);

    // handle shaddow
    auto handleShaddow = handle.expanded(2.f, 3.f).translated(0, 6.f);
    juce::ColourGradient hshaddow(
        juce::Colours::black,
        handleShaddow.getCentreX(), handleShaddow.getY(),
        juce::Colours::transparentBlack,
        handleShaddow.getCentreX(), handleShaddow.getBottom(),
        false
    );
    g.setGradientFill(hshaddow);
    g.fillEllipse(handleShaddow);

    // base
    g.setColour(Colour(0xff333333));
    g.fillRoundedRectangle(handle, 3.f);

    // handle border
    juce::Colour light = juce::Colour::fromFloatRGBA(218 / 255.f, 230 / 255.f, 242 / 255.f, 0.5f);
    juce::Colour dark = juce::Colour::fromFloatRGBA(9 / 255.f, 10 / 255.f, 13 / 255.f, 0.5f);
    juce::ColourGradient grad(
        light, handle.getCentreX(), handle.getY(),
        dark, handle.getCentreX(), handle.getBottom(),
        false
    );
    grad.addColour(0.18, light.withAlpha(0.0f)); // fade out light
    grad.addColour(0.82, dark.withAlpha(0.0f));  // fade in dark
    g.setGradientFill(grad);
    g.fillRoundedRectangle(handle, 3.f);

    // inner handle
    g.setColour(Colour(0xff333333).withMultipliedAlpha(capAlpha));
    g.fillRoundedRectangle(handle.reduced(2.f, 1.f), 2.f);
    juce::Colour topDark = juce::Colour::fromFloatRGBA(9 / 255.f, 10 / 255.f, 13 / 255.f, 0.125f);
    juce::Colour bottomLight = juce::Colour::fromFloatRGBA(218 / 255.f, 230 / 255.f, 242 / 255.f, 0.125f);

    juce::ColourGradient grad2(
        topDark, handle.getCentreX(), handle.getY(),
        bottomLight, handle.getCentreX(), handle.getBottom(),
        false
    );
    grad2.addColour(0.49, topDark.withAlpha(0.0f));     // fade out dark
    grad2.addColour(0.51, bottomLight.withAlpha(0.0f)); // fade in light
    g.setGradientFill(grad2);
    g.fillRoundedRectangle(handle.reduced(2.f, 1.f), 2.f);

    // handle line
    //g.setColour(Colour);
    //g.fillRoundedRectangle(handle.getCentreX() - 1.f, handle.getY(), 2.f, handle.getHeight(), 1.f);
    /////////////////////////////////

    drawLabel(g, editingMod
        ? editor.audioProcessor.params.getRawParameterValue(modId)->load()
        : param->convertFrom0to1(normValue)
    );

    if (showDragAndDrop) {
        g.setColour(dragAndDropColour);
        g.fillRoundedRectangle(bounds, 4.f);
    }
}

void HSlider::drawLabel(juce::Graphics& g, float slider_val)
{
    (void)g;
    if (mouse_down) {
        juce::String valueLabel = "";
        if (editingMod) {
            valueLabel = juce::String(std::round(slider_val * 100)) + " %";
        }
        else if (format == Format::dB) {
            if (slider_val <= 0.f) {
                valueLabel = "-Inf";
            }
            else {
                auto db = 20.0f * std::log10(slider_val);
                std::stringstream ss;
                ss << std::fixed << std::setprecision(1) << db << " dB";
                valueLabel = ss.str();
            }
        }
        else if (format == Format::ABMix) {
            valueLabel = std::to_string((int)((1 - slider_val) * 100)) + ":" + std::to_string((int)(slider_val * 100));
        }
        else if (format == Format::Percent) {
            valueLabel = std::to_string((int)std::round((slider_val * 100))) + " %";
        }
        if (showLabelPrefix) {
            valuePopup->setText(name + "\n" + valueLabel, juce::dontSendNotification);
        }
        else {
            valuePopup->setText(valueLabel, juce::dontSendNotification);
        }
        positionValuePopup();

    }
}

void HSlider::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) {
        editor.showParamContextMenu(this);
        return;
    }

    editor.audioProcessor.undomgr->createUndo();

    if (e.mods.isCommandDown() && modId.isEmpty()) {
        editor.quickConnect(paramId);
    }

    auto distY = e.y;
    editingMod = (e.mods.isCommandDown() || distY <= 4.0) && modId.isNotEmpty();

    // initial slider click sets slider pos
    if (!editingMod) {
        float mouseX = juce::jlimit(track.getX(), track.getRight(), (float)e.x);
        float norm = (mouseX - track.getX()) / track.getWidth();
        norm = juce::jlimit(0.0f, 1.0f, norm);
        cur_normed_value = norm;
    }

    UIUtils::startUnboundedMouse(*this, e, editor.audioProcessor.unboundedMouse);
    mouse_down = true;
    auto param = editor.audioProcessor.params.getParameter(editingMod ? modId : paramId);
    if (editingMod)
        cur_normed_value = param->getValue();
    last_mouse_position = e.getPosition();
    start_mouse_pos = juce::Desktop::getInstance().getMousePosition();
    repaint();
    param->beginChangeGesture();                                   // parent component

    if (!valuePopup) {
        valuePopup = std::make_unique<juce::Label>();
        valuePopup->setJustificationType(juce::Justification::centred);
        valuePopup->setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.8f));
        valuePopup->setColour(juce::Label::textColourId, juce::Colours::white);
        valuePopup->setBorderSize(juce::BorderSize<int>(2));
        valuePopup->setSize(60, showLabelPrefix ? 40 : 25);
    }

    auto* top = getTopLevelComponent();
    if (valuePopup->getParentComponent() != top)
        top->addAndMakeVisible(*valuePopup);

    positionValuePopup();
}

void HSlider::positionValuePopup()
{
    auto* top = getTopLevelComponent();
    auto target = top->getLocalArea(this, handle.translated(0.f, (float)(-valuePopup->getHeight())));
    valuePopup->setTopLeftPosition((int)(target.getCentreX() - valuePopup->getWidth() / 2),
        (int)(target.getY() + target.getHeight() / 2 - valuePopup->getHeight() / 2));
}

void HSlider::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()) {
        return; // prevent crash, param is already mutating
    }

    editor.audioProcessor.undomgr->createUndo();

    if (e.mods.isCommandDown() && modId.isEmpty()) {
        editor.quickConnect(paramId);
    }

    auto distY = e.y;
    auto modEdit = (e.mods.isCommandDown() || distY < 7.0) && modId.isNotEmpty();

    auto speed = e.mods.isShiftDown() ? UIUtils::WHEEL_SPEED_FINE : UIUtils::WHEEL_SPEED;
    auto param = editor.audioProcessor.params.getParameter(modEdit ? modId : paramId);
    auto slider_change = UIUtils::wheelChange(wheel, param, wheelAccum, speed);
    if (slider_change == 0.f) return;
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->getValue() + slider_change);
    while (slider_change > 0.0f && param->getValue() == 0.0f) { // FIX wheel not working when value is zero due to param skew
        slider_change += UIUtils::WHEEL_SPEED;
        param->setValueNotifyingHost(param->getValue() + slider_change);
    }
    param->endChangeGesture();
}

void HSlider::mouseUp(const juce::MouseEvent& e)
{
    if (!mouse_down) return;
    valuePopup.reset();
    mouse_down = false;
    if (UIUtils::stopUnboundedMouse(*this, e, editor.audioProcessor.unboundedMouse))
    {
        if (editingMod)
            juce::Desktop::getInstance().setMousePosition(start_mouse_pos);
        else
            juce::Desktop::getInstance().setMousePosition(localPointToGlobal(handle.getCentre().roundToInt()));
    }
    repaint();
    auto param = editor.audioProcessor.params.getParameter(editingMod ? modId : paramId);

    // one last update since cur_normed_value may not be applied by not dragging
    // cur_normed_value is updated right at mouse down
    // this used to be a direct update to the param on mouse down unfortunately led to rare crashes
    if (!editingMod)
        param->setValueNotifyingHost(cur_normed_value);

    param->endChangeGesture();
    editingMod = false;

    // disconnect if mod range is less than 1%
    if (modId.isNotEmpty()) {
        auto modval = editor.audioProcessor.params.getRawParameterValue(modId)->load();
        if (std::fabs(modval) < 0.01f) {
            editor.audioProcessor.modulation->disconnectSelectedMod(paramId);
            editingMod = false;
        }
    }
}

void HSlider::mouseDoubleClick(const juce::MouseEvent& e) {
    editor.audioProcessor.undomgr->createUndo();
    auto distY = e.y;
    auto modEdit = (e.mods.isCommandDown() || distY < 7.0) && modId.isNotEmpty();

    if (modEdit) {
        editor.audioProcessor.modulation->disconnectSelectedMod(paramId);
        editingMod = false;
        return;
    }

    auto param = editor.audioProcessor.params.getParameter(modEdit ? modId : paramId);
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->getDefaultValue());
    param->endChangeGesture();
}

void HSlider::mouseDrag(const juce::MouseEvent& e) {
    if (!mouse_down) return;
    auto change = e.getPosition() - last_mouse_position;
    last_mouse_position = e.getPosition();
    auto speed = (e.mods.isShiftDown() ? 40.0f : 4.0f) * pixels_per_percent;
    auto slider_change = float(change.getX() - change.getY()) / speed;
    cur_normed_value += slider_change;
    cur_normed_value = std::clamp(cur_normed_value, 0.f, 1.f);
    auto param = editor.audioProcessor.params.getParameter(editingMod ? modId : paramId);

    param->setValueNotifyingHost(cur_normed_value);
}

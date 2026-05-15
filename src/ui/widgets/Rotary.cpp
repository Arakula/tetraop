#include "Rotary.h"
#include "../../PluginEditor.h"
#include "../../Globals.h"

Rotary::Rotary(TetraOPAudioProcessorEditor& e, juce::String paramId, juce::String name, Format format, bool isSymmetric)
    : ModulatedParam(paramId)
    , editor(e)
    , name(name)
    , format(format)
    , isSymmetric(isSymmetric)
{
    editor.audioProcessor.params.addParameterListener(paramId, this);
}

Rotary::~Rotary()
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

void Rotary::setParamId(String pid)
{
    if (paramId.isNotEmpty()) {
        editor.audioProcessor.params.removeParameterListener(paramId, this);
    }

    paramId = pid;
    editor.audioProcessor.params.addParameterListener(paramId, this);
    setName(paramId);
    repaint();
}

void Rotary::setModId(String mid)
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

void Rotary::parameterChanged(const juce::String& parameterID, float newValue)
{
    (void)parameterID;
    (void)newValue;
    juce::MessageManager::callAsync([this] { repaint(); });
}

void Rotary::paint(Graphics& g) {
    auto param = editor.audioProcessor.params.getParameter(paramId);
    auto normValue = param->getValue();

    drawRotary(g, normValue);

    if (modId.isNotEmpty()) {
        drawModRange(g, normValue);
    }

    if (modulated) {
        drawModValue(g, normValue);
    }

    drawLabel(g, editingMod
        ? editor.audioProcessor.params.getRawParameterValue(modId)->load()
        : param->convertFrom0to1(normValue)
    );
}

void Rotary::setSmall()
{
    radius = KNOB_RADIUS_SM;
    yoffset = KNOB_YOFFSET_SM;
    value_offset = KNOB_VALUE_OFFSET_SM;
    isSmall = true;
}

void Rotary::setMatrixBtn()
{
    radius = KNOB_RADIUS_SM;
    yoffset = KNOB_YOFFSET_SM;
    value_offset = KNOB_VALUE_OFFSET_SM;
    isSmall = true;
    isMatrixBtn = true;
}

void Rotary::setDark()
{
    baseColor = COLOR_KNOB().darker(0.5f);
    //labelColor = COLOR_KNOB_LABEL().darker(0.5f);
    isDark = true;
}

void Rotary::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) {
        editor.showParamContextMenu(this);
        return;
    }

    editor.audioProcessor.undomgr->createUndo();

    if (onMouseDown)
        onMouseDown();

    if (e.mods.isCommandDown() && modId.isEmpty()) {
        editor.quickConnect(paramId);
    }

    float dx = (float)(e.x - getWidth() * 0.5f);
    float dy = (float)(e.y - getHeight() * 0.5f - yoffset);
    float dist_center = std::sqrt(dx * dx + dy * dy);

    UIUtils::startUnboundedMouse(*this, e, editor.audioProcessor.unboundedMouse);
    mouse_down = true;

    editingMod = (e.mods.isCommandDown() || dist_center >= radius + mod_offset) && modId.isNotEmpty();
    auto param = editor.audioProcessor.params.getParameter(editingMod ? modId : paramId);
    auto cur_val = param->getValue();
    cur_normed_value = cur_val;
    last_mouse_position = e.getPosition();
    setMouseCursor(MouseCursor::NoCursor);
    start_mouse_pos = Desktop::getInstance().getMousePosition();
    repaint();
    param->beginChangeGesture();
}

void Rotary::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (mouse_down) return; // prevent crash, param is already mutating

    editor.audioProcessor.undomgr->createUndo();

    if (e.mods.isCommandDown() && modId.isEmpty()) {
        editor.quickConnect(paramId);
    }

    float dx = (float)(e.x - getWidth() * 0.5f);
    float dy = (float)(e.y - getHeight() * 0.5f - yoffset);
    float dist_center = std::sqrt(dx * dx + dy * dy);
    auto modedit = (e.mods.isCommandDown() || dist_center >= radius + mod_offset) && modId.isNotEmpty();

    auto speed = (e.mods.isShiftDown() ? 0.01f : 0.05f);
    auto slider_change = wheel.deltaY > 0 ? speed : wheel.deltaY < 0 ? -speed : 0;

    auto param = editor.audioProcessor.params.getParameter(modedit ? modId : paramId);
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->getValue() + slider_change);
    while (wheel.deltaY > 0.0f && param->getValue() == 0.0f) { // FIX wheel not working when value is zero, first step takes more than 0.05%
        slider_change += 0.05f;
        param->setValueNotifyingHost(param->getValue() + slider_change);
    }
    param->endChangeGesture();
}

void Rotary::mouseUp(const juce::MouseEvent& e) {
    if (!mouse_down) return;

    if (onMouseUp)
        onMouseUp();

    mouse_down = false;
    if (UIUtils::stopUnboundedMouse(*this, e, editor.audioProcessor.unboundedMouse))
        juce::Desktop::getInstance().setMousePosition(start_mouse_pos);
    repaint();

    auto param = editor.audioProcessor.params.getParameter(editingMod ? modId : paramId);
    param->endChangeGesture();
    editingMod = false;

    // disconnect if mod range is less than 1%
    if (modId.isNotEmpty()) {
        auto modval = editor.audioProcessor.params.getRawParameterValue(modId)->load();
        if (std::fabs(modval) <= 0.005f) {
            editor.audioProcessor.modulation->disconnectSelectedMod(paramId.toStdString());
            editingMod = false;
        }
    }
}

void Rotary::mouseDoubleClick(const juce::MouseEvent& e) {
    float dx = (float)(e.x - getWidth() * 0.5f);
    float dy = (float)(e.y - getHeight() * 0.5f - yoffset);
    float dist_center = std::sqrt(dx * dx + dy * dy);
    auto modedit = (e.mods.isCommandDown() || dist_center >= radius + mod_offset) && modId.isNotEmpty();

    editor.audioProcessor.undomgr->createUndo();

    if (modedit) {
        editor.audioProcessor.modulation->disconnectSelectedMod(paramId.toStdString());
        editingMod = false;
        return;
    }

    auto param = editor.audioProcessor.params.getParameter(paramId);
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->getDefaultValue());
    param->endChangeGesture();
}

void Rotary::mouseDrag(const juce::MouseEvent& e) {
    if (!mouse_down) return;
    auto change = e.getPosition() - last_mouse_position;
    last_mouse_position = e.getPosition();
    auto speed = (e.mods.isShiftDown() ? 40.0f : 4.0f) * pixels_per_percent;
    auto slider_change = float(change.getX() - change.getY()) / speed;
    cur_normed_value += slider_change;
    auto param = editor.audioProcessor.params.getParameter(editingMod ? modId : paramId);

    if (format == Format::PitchSemis && !e.mods.isShiftDown() && !editingMod) {
        // snap values for pitch knob
        auto val = param->convertFrom0to1(cur_normed_value);
        param->setValueNotifyingHost(param->convertTo0to1(std::round(val)));
    }
    else {
        param->setValueNotifyingHost(cur_normed_value);
    }
}

void Rotary::mouseEnter(const juce::MouseEvent& e)
{
    (void)e;
    if (onMouseEnter)
        onMouseEnter();
}
void Rotary::mouseExit(const juce::MouseEvent& e)
{
    (void)e;
    if (onMouseExit)
        onMouseExit();
}

void Rotary::drawRotary(juce::Graphics& g, float slider_pos) {
    auto bounds = getLocalBounds();
    auto center = bounds.toFloat().getCentre().translated(0, yoffset);

    // draw knob shadow
    juce::DropShadow shadow(Colours::black.withAlpha(KNOB_SHADOW_ALPHA), 4, { 0, 4 });
    juce::Path shadowPath;
    shadowPath.addEllipse(center.x - radius, center.y - radius, radius * 2, radius * 2);
    shadow.drawForPath(g, shadowPath);

    // draw knob base
    auto circle = Rectangle<float>({ center.x - radius, center.y - radius, radius * 2, radius * 2 });
    if (!isMatrixBtn)
    {
        g.setColour(COLOR_BEVEL());
        g.fillEllipse(circle.expanded(2));
        g.setColour(COLOR_KNOB());
        if (baseColor != Colours::transparentBlack)
            g.setColour(baseColor);

        g.fillEllipse(circle);
        juce::ColourGradient grad1(
            Colours::white.withAlpha(.17f),
            circle.getX(), circle.getY(),
            Colours::white.withAlpha(.0f),
            circle.getX(), circle.getBottom(),
            false
        );
        grad1.addColour(0.5, Colours::white.withAlpha(.17f));
        g.setGradientFill(grad1);
        g.fillEllipse(circle);
        g.setColour(COLOR_KNOB());
        if (baseColor != Colours::transparentBlack)
            g.setColour(baseColor);

        auto innerc = circle.reduced(2);
        g.fillEllipse(innerc);
        juce::ColourGradient grad2(
            Colours::black.withAlpha(0.12f),
            innerc.getX(), innerc.getY(),
            Colours::white.withAlpha(0.12f),
            innerc.getX(), innerc.getBottom(),
            false
        );
        grad2.addColour(0.27, Colours::black.withAlpha(0.12f));
        g.setGradientFill(grad2);
        g.fillEllipse(innerc);
    }

    // draw outer arc
    if (drawArc) {
        juce::Path arc;
        arc.addCentredArc(center.x, center.y, radius + value_offset, radius + value_offset, 0, -DEG130, DEG130, true);
        g.setColour(COLOR_KNOB_ARC().darker(isMatrixBtn || isDark ? 0.5f : 0.f));
        g.strokePath(arc, PathStrokeType(value_thickness));
    }

    // draw active line
    g.setColour(colorValue);
    const float angle = -DEG130 + slider_pos * (DEG130 - -DEG130);
    const bool valueIsZero = (slider_pos == 0.f && !invertValue) || (slider_pos == 1.0f && invertValue);
    if (isEnabled()) {
        if (drawValue && ((isSymmetric && slider_pos != 0.5f) || (!isSymmetric && !valueIsZero))) {
            juce::Path arc2;
            arc2.addCentredArc(center.x, center.y, radius + value_offset, radius + value_offset, 0, isSymmetric ? 0 : invertValue ? DEG130 : -DEG130, angle, true);
            g.strokePath(arc2, PathStrokeType(value_thickness));
        }
    }

    // draw handle
    juce::Path handle;
    handle.addRoundedRectangle(-1.f, -radius+2.f, 2.f, isSmall ? 8.f : 12.f, 1.f);
    g.setColour(COLOR_KNOB_HANDLE());
    g.fillPath (handle, juce::AffineTransform::rotation (angle).translated(bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f + yoffset));

    // draw drag and drop zone
    if (showDragAndDrop && isEnabled()) {
        g.setColour(dragAndDropColour);
        g.fillEllipse(bounds.getWidth() / 2.f - radius, bounds.getHeight() / 2.f - radius + yoffset, radius * 2.f, radius * 2.f);
    }
}

void Rotary::drawModValue(juce::Graphics& g, float slider_pos) const {
    if (!isEnabled()) return;
    auto bounds = getBounds();
    const float rad = radius + mod_value_offset;
    const float value = voiceActive ? modValue : slider_pos;
    const float mod_angle = -DEG130 + value * (DEG130 - -DEG130);

    float cx = bounds.getWidth() / 2.0f;
    float cy = bounds.getHeight() / 2.0f + yoffset;

    // Compute dot position at mod_angle
    auto r = mod_value_radius;
    float dotX = cx + std::cos(mod_angle - juce::MathConstants<float>::halfPi) * (rad + r);
    float dotY = cy + std::sin(mod_angle - juce::MathConstants<float>::halfPi) * (rad + r);

    // Draw small circle radius 1.0f
    g.setColour(COLOR_PANEL());
    g.fillEllipse(dotX - r * 2, dotY - r * 2, r * 4, r * 4);
    g.setColour(colorModValue);
    g.fillEllipse(dotX - r, dotY - r, r*2, r*2);
}

void Rotary::drawModRange(juce::Graphics& g, float slider_pos) const {
    if (!isEnabled()) return;
    auto bounds = getBounds();
    const float rad = radius + mod_offset;
    const float slider_angle = -DEG130 + slider_pos * (DEG130 - -DEG130);
    float amount = editor.audioProcessor.params.getRawParameterValue(modId)->load();
    if (amount == 0.f) return;
    if (modBipolar) amount *= 0.5;
    Colour c = modColor;

    float mod_angle = -DEG130 + std::fmax(0.f, std::fmin(1.f, slider_pos + amount)) * (DEG130 - -DEG130);
    g.setColour(Colour(c));

    juce::Path arc;
    arc.addCentredArc(bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f + yoffset, rad, rad, 0, slider_angle, mod_angle, true);
    g.strokePath(arc, PathStrokeType(mod_thickness));

    if (modBipolar) {
        mod_angle = -DEG130 + std::fmax(0.f, std::fmin(1.f, slider_pos - amount)) * (DEG130 - -DEG130);
        g.setColour(Colour(c).withMultipliedAlpha(0.5f));

        juce::Path arc2;
        arc2.addCentredArc(bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f + yoffset, rad, rad, 0, slider_angle, mod_angle, true);
        g.strokePath(arc2, PathStrokeType(mod_thickness));
    }
}

void Rotary::drawLabel(juce::Graphics& g, float slider_val)
{
    if (!drawTextLabel) return;
    String text = name;

    if (mouse_down || mouse_hover || forceLabelShowValue) {
        if (editingMod) {
            text = String(std::round(slider_val * 100)) + " %";
        }
        else if (format == Format::Percent) text = std::to_string((int)std::round(slider_val * 100)) + " %";
        else if (format == Format::Percent1f) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << (slider_val * 100.0f);
            text = oss.str() + " %";
        }
        else if (format == Format::millis) {
            if (slider_val < 1000.0f) {
                text = std::to_string((int)slider_val) + " ms";
            }
            else {
                std::stringstream ss;
                ss << std::fixed << std::setprecision(2) << (slider_val / 1000.f) << " s";
                text = ss.str();
            }
        }
        else if (format == Format::Hz) {
            std::stringstream ss;
            if (slider_val >= 1000.f) {
                ss << std::fixed << std::setprecision(1) << (slider_val / 1000.f) << " kHz";
            }
            else {
                ss << std::fixed << std::setprecision(0) << slider_val << " Hz";
            }
            text = ss.str();
        }
        else if (format == Format::Hz1f) {
            std::stringstream ss;
            if (slider_val >= 1000.f) {
                ss << std::fixed << std::setprecision(1) << (slider_val / 1000.f) << " kHz";
            }
            else if (slider_val < 0.1) {
                ss << std::fixed << std::setprecision(2) << slider_val << " Hz";
            }
            else {
                ss << std::fixed << std::setprecision(1) << slider_val << " Hz";
            }
            text = ss.str();
        }
        else if (format == Format::float1) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << slider_val;
            text = ss.str();
        }
        else if (format == Format::float2) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << slider_val;
            text = ss.str();
        }
        else if (format == Format::float3) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(3) << slider_val;
            text = ss.str();
        }
        else if (format == Format::float2_100) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << slider_val * 100;
            text = ss.str();
        }
        else if (format == Format::seconds2f) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << slider_val << " s";
            text = ss.str();
        }
        else if (format == Format::seconds1f) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << slider_val << " s";
            text = ss.str();
        }
        else if (format == Format::ABMix) text = std::to_string((int)((1 - slider_val) * 100)) + ":" + std::to_string((int)(slider_val * 100));
        else if (format == Format::dB) text = std::to_string((int)slider_val) + " dB";
        else if (format == Format::gain2dB) {
            if (slider_val == 0) {
                text = "-Inf";
            } else {
                auto db = 20.0f * std::log10(slider_val);
                text = String(db, 1) + " dB";
            }
        }
        else if (format == Format::Pan) {
            if (std::abs(slider_val - 0.5) < 1e-4) {
                text = "C";
            }
            else {
                auto p = (slider_val * 2.f) - 1.f;
                text = String(std::round(std::abs(p) * 100.f)) + (p < 0.f ? "L" : "R");
            }
        }
        else if (format == Format::FilterLPHP) {
            if (slider_val == 0.0) {
                text = "Off";
            }
            else {
                std::stringstream ss;
                double freq = 20.0 * std::pow(1000, slider_val < 0.0 ? 1 + slider_val : slider_val); // map 1..0 to 20..20000

                ss << (slider_val < 0.0 ? "LP " : "HP ");
                if (freq >= 1000.0) {
                    ss << std::fixed << std::setprecision(1) << (freq / 1000.0) << " kHz";
                }
                else {
                    ss << std::fixed << std::setprecision(0) << freq << " Hz";
                }

                text = ss.str();
            }
        }
        else if (format == Format::Integer)
        {
            text = String(std::round(slider_val));
        }
        else if (format == Format::PitchSemis) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(0) << slider_val;
            text = ss.str();
        }
        else if (format == Format::RateTempo) {
            if (slider_val == 0.f) text = "16 Bars";
            else if (slider_val == 1.f) text = "8 Bars";
            else if (slider_val == 2.f) text = "4 Bars";
            else if (slider_val == 3.f) text = "2 Bars";
            else if (slider_val == 4.f) text = "Bar";
            else if (slider_val == 5.f) text = "1/2";
            else if (slider_val == 6.f) text = "1/4";
            else if (slider_val == 7.f) text = "1/8";
            else if (slider_val == 8.f) text = "1/16";
            else if (slider_val == 9.f) text = "1/32";
            else if (slider_val == 10.f) text = "1/64";
        }
        else if (format == Format::DelayTempo) {
            if (slider_val == 0.f) text = "Off";
            else if (slider_val == 1.f) text = "1/64";
            else if (slider_val == 2.f) text = "1/32";
            else if (slider_val == 3.f) text = "1/16";
            else if (slider_val == 4.f) text = "1/8";
            else if (slider_val == 5.f) text = "1/4";
            else if (slider_val == 6.f) text = "1/2";
            else if (slider_val == 7.f) text = "Bar";
            else if (slider_val == 8.f) text = "2 Bars";
            else if (slider_val == 9.f) text = "4 Bars";
            else if (slider_val == 10.f) text = "8 Bars";
            else if (slider_val == 11.f) text = "16 Bars";
        }
        else if (format == Format::TremoloTempo) {
            if (slider_val == 0) text = "2 Bar";
            if (slider_val == 1) text = "Bar";
            if (slider_val == 2) text = "1/2";
            if (slider_val == 3) text = "1/4";
            if (slider_val == 4) text = "1/8";
            if (slider_val == 5) text = "1/16";
            if (slider_val == 6) text = "1/32";
            if (slider_val == 7) text = "1/64";
            if (slider_val == 8) text = "1/128";
        }
        else if (format == Format::OSCMorphA) {
            auto ntables = editor.audioProcessor.tablesMgr->wavetables[0].numTables;
            auto idx = std::min(ntables - 1, int(float(ntables) * slider_val)) + 1;
            text = String(idx);
        }
        else if (format == Format::OSCMorphB) {
            auto ntables = editor.audioProcessor.tablesMgr->wavetables[1].numTables;
            auto idx = std::min(ntables - 1, int(float(ntables) * slider_val)) + 1;
            text = String(idx);
        }
        else if (format == Format::OSCMorphC) {
            auto ntables = editor.audioProcessor.tablesMgr->wavetables[2].numTables;
            auto idx = std::min(ntables - 1, int(float(ntables) * slider_val)) + 1;
            text = String(idx);
        }
        else if (format == Format::OSCMorphD) {
            auto ntables = editor.audioProcessor.tablesMgr->wavetables[3].numTables;
            auto idx = std::min(ntables - 1, int(float(ntables) * slider_val)) + 1;
            text = String(idx);
        }
        else if (format == Format::OSCMorphA2f) {
            auto ntables = editor.audioProcessor.tablesMgr->wavetables[0].numTables;
            auto idx = ntables * slider_val;
            text = String(idx,2);
        }
        else if (format == Format::OSCMorphB2f) {
            auto ntables = editor.audioProcessor.tablesMgr->wavetables[1].numTables;
            auto idx = ntables * slider_val;
            text = String(idx, 2);
        }
        else if (format == Format::OSCMorphC2f) {
            auto ntables = editor.audioProcessor.tablesMgr->wavetables[2].numTables;
            auto idx = ntables * slider_val;
            text = String(idx, 2);
        }
        else if (format == Format::OSCMorphD2f) {
            auto ntables = editor.audioProcessor.tablesMgr->wavetables[3].numTables;
            auto idx = ntables * slider_val;
            text = String(idx, 2);
        }
        else if (format == Format::secondsMillis) {
            std::stringstream ss;
            if (slider_val < 1.f)
                ss << std::fixed << std::setprecision(0) << slider_val * 1000.f << " ms";
            else
                ss << std::fixed << std::setprecision(2) << slider_val << " s";
            text = ss.str();
        }
        else if (format == Format::secondsSubMillis) {
            std::stringstream ss;
            if (slider_val < 0.01)
                ss << std::fixed << std::setprecision(1) << slider_val * 1000.f << " ms";
            else if (slider_val < 1)
                ss << std::fixed << std::setprecision(0) << slider_val * 1000.f << " ms";
            else
                ss << std::fixed << std::setprecision(2) << slider_val << " s";
            text = ss.str();
        }
        else if (format == Format::TimeFactor) {
            auto v = std::pow(4.f, 1.f * slider_val);
            text = String((int)std::round(v * 100.f)) + " %";
        }
        else if (format == Format::Choice) {
            text = editor.audioProcessor.params.getParameter(paramId)->getCurrentValueAsText();
        }
        else if (format == Format::VerbPredelay) {
            text = juce::String(std::round(slider_val * 1000.f)) + " ms";
        }
    }

    g.setColour(Colour(COLOR_KNOB_LABEL()));
    g.setFont(labelSize);
    g.drawText(text, 0, getHeight() - (int)(labelSize - KNOB_LABEL_YOFFSET), getWidth(), (int)labelSize, juce::Justification::centred, true);
}
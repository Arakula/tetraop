#include "TimePicker.h"
#include "../../PluginEditor.h"

TimePicker::TimePicker(TetraOPAudioProcessorEditor& e, juce::String _paramId)
	: ModulatedParam(_paramId)
	, editor(e)
{
	editor.audioProcessor.params.addParameterListener(_paramId, this);
}

TimePicker::~TimePicker()
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

void TimePicker::setModId(juce::String mid)
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

void TimePicker::parameterChanged(const juce::String& parameterID, float newValue)
{
	(void)parameterID;
	(void)newValue;
	juce::MessageManager::callAsync([this] { repaint(); });
}

juce::String TimePicker::getSyncText(float value)
{
	auto i = (int)value;
	juce::String text = "1/128";
	if (i == 0) text = "2 Bar";
	if (i == 1) text = "Bar";
	if (i == 2) text = "1/2";
	if (i == 3) text = "1/4";
	if (i == 4) text = "1/8";
	if (i == 5) text = "1/16";
	if (i == 6) text = "1/32";
	if (i == 7) text = "1/64";

	if (mode == 2) text += " t";
	if (mode == 3) text += " .";

	return text;
}

void TimePicker::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds().toFloat().reduced(1.f);
	UIUtils::drawBevelLight(g, bounds, false);

	if (!isEnabled())
		return;

	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(juce::FontOptions(15.f));

	if (editingMod) {
		auto modval = editor.audioProcessor.params.getRawParameterValue(modId)->load();
		g.drawText(juce::String(std::round(modval * 100)) + " %", (int)bounds.getX(),
			(int)bounds.getY(), (int)bounds.getWidth(), (int)bounds.getHeight(), juce::Justification::centred, false);
	}
	else {
		auto value = editor.audioProcessor.params.getRawParameterValue(paramId)->load();
        auto text = mode > 0 ? getSyncText(value)
            : value >= 1.f ? juce::String(std::round(value * 100.f) / 100.f) + " s"
            : juce::String(std::round(value * 1000.f)) + " ms";
		g.drawText(text, bounds, juce::Justification::centred, false);
	}

	auto modLane = bounds.withHeight(2.f).withBottomY(bounds.getBottom() - 2).toFloat();
	auto param = editor.audioProcessor.params.getParameter(paramId);
	auto normValue = param->getValue();

	g.saveState();
	juce::Path pp;
	pp.addRoundedRectangle(bounds.reduced(1), 3.f, 3.f);
	g.reduceClipRegion(pp);

	// draw mod range
	if (modId.isNotEmpty()) {
		float amount = editor.audioProcessor.params.getRawParameterValue(modId)->load();
		if (modBipolar) {
			amount *= 0.5f;
		}
		float handleX = modLane.getX() + normValue * modLane.getWidth();
		float modOffset = amount * modLane.getWidth();
		float modX = std::clamp(handleX + modOffset, modLane.getX(), modLane.getX() + modLane.getWidth());

		float modRight = std::fmin(handleX, modX);
		float modWidth = std::fabs(modX - handleX);

		g.setColour(modColor);
		g.fillRect(modRight, modLane.getY(), modWidth, modLane.getHeight());
		if (modBipolar) {
			g.setColour(modColor.withMultipliedAlpha(0.5f));
			float inverseX = std::clamp(handleX - modOffset, modLane.getX(), modLane.getX() + modLane.getWidth());

			float invRight = std::fmin(handleX, inverseX);
			float invWidth = std::fabs(inverseX - handleX);

			g.fillRect(invRight, modLane.getY(), invWidth, modLane.getHeight());
		}
	}

	// draw mod value
	if (modulated) {
		const float value = voiceActive ? modValue : normValue;
		auto r = 1.5f;
		float dotX = modLane.getX() + (modLane.getWidth() * value);
		float dotY = modLane.getY() + modLane.getHeight() / 2.f;

		g.setColour(COLOR_BEVEL());
		g.fillEllipse(dotX - r * 2, dotY - r * 2, r * 4, r * 4);
		g.setColour(COLOR_ACTIVE());
		g.fillEllipse(dotX - r, dotY - r, r * 2, r * 2);
	}

	g.restoreState();

	if (showDragAndDrop) {
		g.setColour(dragAndDropColour);
		g.fillRoundedRectangle(bounds.toFloat(), 6.f);
	}
}

void TimePicker::mouseDown(const juce::MouseEvent& e)
{
	if (!isEnabled())
		return;

	if (e.mods.isRightButtonDown()) {
		editor.showParamContextMenu(this);
		return;
	}

    editor.audioProcessor.undomgr->createUndo();

	if (e.mods.isCommandDown() && modId.isEmpty()) {
		editor.quickConnect(paramId);
	}

	auto distY = e.y;
	editingMod = (e.mods.isCommandDown() || distY > getLocalBounds().getBottom() - 10) && modId.isNotEmpty();

	mouse_down = true;
	UIUtils::startUnboundedMouse(*this, e, editor.audioProcessor.unboundedMouse);
	auto param = editor.audioProcessor.params.getParameter(editingMod ? modId : paramId);
	cur_normed_value = param->getValue();
	start_mouse_pos = juce::Desktop::getInstance().getMousePosition();
	last_mouse_position = e.getPosition();
	param->beginChangeGesture();
}

void TimePicker::mouseUp(const juce::MouseEvent& e)
{
	if (!mouse_down) return;
	mouse_down = false;
	if (UIUtils::stopUnboundedMouse(*this, e, editor.audioProcessor.unboundedMouse))
        juce::Desktop::getInstance().setMousePosition(start_mouse_pos);
	auto param = editor.audioProcessor.params.getParameter(editingMod ? modId : paramId);
	param->endChangeGesture();
	editingMod = false;
}

void TimePicker::mouseDrag(const juce::MouseEvent& e)
{
	if (!mouse_down) return;
	auto change = e.getPosition() - last_mouse_position;
	last_mouse_position = e.getPosition();
	auto speed = (e.mods.isShiftDown() ? 40.0f : 4.0f) * pixels_per_percent;
	auto slider_change = float(change.getX() - change.getY()) / speed;
	cur_normed_value += slider_change;
	auto param = editor.audioProcessor.params.getParameter(editingMod ? modId : paramId);
	param->setValueNotifyingHost(cur_normed_value);
}

void TimePicker::mouseDoubleClick(const juce::MouseEvent& e)
{
    editor.audioProcessor.undomgr->createUndo();

	auto distY = e.y;
	auto modEdit = (e.mods.isCommandDown() || distY > getLocalBounds().getBottom() - 10) && modId.isNotEmpty();

	if (modEdit) {
		editor.audioProcessor.modulation->disconnectSelectedMod(paramId);
		editingMod = false;
		return;
	}

	auto param = editor.audioProcessor.params.getParameter(paramId);
	param->setValueNotifyingHost(param->getDefaultValue());
}

void TimePicker::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
	if (e.mods.isLeftButtonDown() || e.mods.isRightButtonDown()) {
		return; // prevent crash, param is already mutating
	}

    editor.audioProcessor.undomgr->createUndo();

	if (e.mods.isCommandDown() && modId.isEmpty()) {
		editor.quickConnect(paramId);
	}

	auto distY = e.y;
	auto modEdit = (e.mods.isCommandDown() || distY > getLocalBounds().getBottom() - 10) && modId.isNotEmpty();

	if (modEdit) {
		auto speed = e.mods.isShiftDown() ? UIUtils::WHEEL_SPEED_FINE : UIUtils::WHEEL_SPEED;
		auto param = editor.audioProcessor.params.getParameter(modId);
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
	else {
		auto step = UIUtils::wheelStep(wheel, wheelAccum);
		if (step == 0) return;
		auto param = editor.audioProcessor.params.getParameter(paramId);
		auto val = param->convertFrom0to1(param->getValue());
		param->setValueNotifyingHost(param->convertTo0to1(val + (float)step));
	}
}

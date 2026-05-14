#include "./PowerCurve.h"
#include "../../PluginEditor.h"

PowerCurve::PowerCurve(TetraOPAudioProcessorEditor& e, juce::String pid, bool bipolar)
	: ModulatedParam(pid)
	, bipolar(bipolar)
	, editor(e)
{
	setName(paramId);
	editor.audioProcessor.params.addParameterListener(paramId, this);
}

PowerCurve::~PowerCurve()
{
	editor.audioProcessor.params.removeParameterListener(paramId, this);
}

void PowerCurve::parameterChanged(const juce::String& parameterID, float newValue)
{
	(void)parameterID;
	(void)newValue;
	juce::MessageManager::callAsync([this]() { repaint(); });
}

void PowerCurve::setParam(juce::String pid)
{
	editor.audioProcessor.params.removeParameterListener(paramId, this);
	paramId = pid;
	editor.audioProcessor.params.addParameterListener(paramId, this);
	setName(paramId);
	repaint();
}

void PowerCurve::setModId(juce::String mid)
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

void PowerCurve::paint(juce::Graphics& g)
{
	auto b = getLocalBounds().toFloat();
	UIUtils::drawBevel(g, b.reduced(1.5f), 4.f, COLOR_BACKGROUND());
	b.reduce(4.f, 4.f);

	auto calcY = [this](float x, float ten, float pwr)
		{
			if (bipolar)
				x = x * 2 - 1;

			float sign = x >= 0 ? 1.f : -1.f;
			float a = std::fabs(x);

			float y = ten < 0
				? -1 * (std::pow(1 - a, pwr) - 1)
				: std::pow(a, pwr);

			y *= sign;
			if (bipolar)
				y = (y + 1.0f) * 0.5f;

			return y;
		};

	float tension = editor.audioProcessor.params.getRawParameterValue(paramId)->load();
	float power = pow(1.1f, std::fabs(tension * globals::POWER_CURVE_POWER));

	juce::Path p;
	p.startNewSubPath(b.getX(), b.getBottom());
	auto w = b.getWidth();
	for (int i = 0; i < w; ++i) {
		auto x = i / (float)w;
		auto y = 1 - calcY(x, tension, power);
		p.lineTo(b.getX() + i, b.getY() + b.getHeight() * y);
	}
	g.setColour(COLOR_ACTIVE());
	g.strokePath(p, juce::PathStrokeType(1.4f));
}

void PowerCurve::mouseDown(const juce::MouseEvent& e)
{
    editor.audioProcessor.undomgr->createUndo();
	UIUtils::startUnboundedMouse(*this, e, editor.audioProcessor.unboundedMouse);
	mouse_down = true;
	auto param = editor.audioProcessor.params.getParameter(paramId);
	auto cur_val = param->getValue();
	cur_normed_value = cur_val;
	last_mouse_position = e.getPosition();
	start_mouse_pos = juce::Desktop::getInstance().getMousePosition();
	repaint();
	param->beginChangeGesture();
}

void PowerCurve::mouseDrag(const juce::MouseEvent& e)
{
	auto change = e.getPosition() - last_mouse_position;
	last_mouse_position = e.getPosition();
	auto speed = (e.mods.isShiftDown() ? 40.0f : 4.0f) * 100.f;
	auto slider_change = float(change.getX() - change.getY()) / speed;
	cur_normed_value -= slider_change;

	auto val = cur_normed_value;
	if (fabsf(cur_normed_value - 0.5f) <= 0.025f)
		val = 0.5f; // snap curve to zero

	auto param = editor.audioProcessor.params.getParameter(paramId);
	param->setValueNotifyingHost(val);
}

void PowerCurve::mouseUp(const juce::MouseEvent& e)
{
	if (!mouse_down) return;

	mouse_down = false;
	if (UIUtils::stopUnboundedMouse(*this, e, editor.audioProcessor.unboundedMouse))
        juce::Desktop::getInstance().setMousePosition(start_mouse_pos);
	repaint();

	auto param = editor.audioProcessor.params.getParameter(paramId);
	param->endChangeGesture();
}

void PowerCurve::mouseDoubleClick(const juce::MouseEvent&) {
    editor.audioProcessor.undomgr->createUndo();
	auto param = editor.audioProcessor.params.getParameter(paramId);
	param->beginChangeGesture();
	param->setValueNotifyingHost(param->getDefaultValue());
	param->endChangeGesture();
}

void PowerCurve::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
	if (mouse_down) return;
    editor.audioProcessor.undomgr->createUndo();

	auto step = UIUtils::wheelStep(wheel, wheelAccum);
	if (step == 0) return;

	auto stepSize = e.mods.isShiftDown() ? 0.01f : 0.05f;
	auto slider_change = -(float)step * stepSize; // PowerCurve uses inverted convention

	auto param = editor.audioProcessor.params.getParameter(paramId);
	param->beginChangeGesture();
	param->setValueNotifyingHost(param->getValue() + slider_change);
	param->endChangeGesture();
}

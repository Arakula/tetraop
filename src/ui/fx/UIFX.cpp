#include "UIFX.h"
#include "../../PluginEditor.h"

UIFX::UIFX(TetraOPAudioProcessorEditor& e, FX::FXType _type)
		: type(_type)
		, editor(e)
	{
		name = juce::String(FX::FXName[type].data());
		color = FX::getColor(type);
		prefix = juce::String(FX::FXPrefix[type].data());

		onBtn.setName ("onBtn");
		addAndMakeVisible(onBtn);
		onBtn.setAlpha(0.f);
		onBtn.onClick = [this]()
			{
				auto param = editor.audioProcessor.params.getParameter(prefix + "on");
				param->setValueNotifyingHost(param->getValue() > 0.f ? 0.f : 1.f);
			};
	}

UIFX::~UIFX()
{
}

void UIFX::mouseDown(const juce::MouseEvent& e)
{
    editor.audioProcessor.undomgr->createUndo();
	toFront(true);
	dragger.startDraggingComponent(this, e);
}

void UIFX::mouseDrag(const juce::MouseEvent& e)
{
	auto oldBounds = getBounds();
	dragger.dragComponent(this, e, nullptr);
	setTopLeftPosition(std::clamp(getX(), minX, maxX), oldBounds.getY());
	if (onDrag)
		onDrag(this);
}

void UIFX::mouseUp(const juce::MouseEvent&)
{
	if (onDragEnded)
		onDragEnded(this);
}

void UIFX::paint(juce::Graphics& g)
{
	auto b = getLocalBounds().toFloat();
	g.setColour (COLOR_PANEL());
	UIUtils::drawPanel(g, b, true);
	auto bbtn = onBtn.getBounds();
	UIUtils::drawCheckmark(g, bbtn.toFloat(), COLOR_BACKGROUND(), color, on);
	g.setFont(juce::FontOptions(16.f));
	g.setColour(COLOR_KNOB_LABEL());
	auto title = titleOverride.isNotEmpty() ? titleOverride : name;
	g.drawText(title, bbtn.withX(bbtn.getRight()).withWidth(200).toFloat(), juce::Justification::centredLeft);
}

void UIFX::resized()
{
	auto b = getLocalBounds();
	onBtn.setBounds(b.getX(), b.getY(), PANEL_HEADER_HEIGHT, PANEL_HEADER_HEIGHT);
}

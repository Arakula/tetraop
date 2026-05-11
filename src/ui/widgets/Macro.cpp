#include "Macro.h"
#include "../../PluginEditor.h"

Macro::Macro(TetraOPAudioProcessorEditor& e, int index)
	: index(index)
    , editor(e)
	, macroId("macro" + juce::String(index+1))
    , macroIdx(index)
{
	rotary = std::make_unique<Rotary>(editor, macroId, "Macro" + juce::String(index + 1), Rotary::Percent);
	rotary->setSmall();
	rotary->drawTextLabel = false;
	addAndMakeVisible(rotary.get());

	addAndMakeVisible(nameBtn);
	nameBtn.setAlpha(0.f);
	nameBtn.onClick = [this]
		{
            //editor.audioProcessor.undomgr->createUndo();
			editor.showMacroRename(this);
		};

	startTimerHz(30);
	editor.registerModParam(rotary.get());
}

Macro::~Macro()
{
}

void Macro::timerCallback()
{
	auto selectedModId = juce::String(editor.audioProcessor.modulation->selectedMod);
	auto& mod = editor.audioProcessor.modulation->modulators[macroId];

	if (selectedModId == macroId && !selected) {
		selected = true;
		repaint();
	}
	else if (selectedModId != macroId && selected) {
		selected = false;
		repaint();
	}

	if (connections != mod.connections) {
		connections = mod.connections;
		repaint();
	}

	juce::String name = editor.audioProcessor.modulation->macroNames[index];
	if (macroName != name) {
		macroName = name;
		repaint();
	}
}

void Macro::mouseUp(const juce::MouseEvent& e)
{
	(void)e;
	setMouseCursor(juce::MouseCursor::NormalCursor);
	if (editor.audioProcessor.modulation->selectedMod != macroId) {
		editor.audioProcessor.modulation->setSelectedMod(macroId);
	}
}

void Macro::mouseDrag(const juce::MouseEvent& e)
{
	if (!editor.isDragDropModulation && !getLocalBounds().contains(e.getPosition())) {
		editor.startDragDrop(macroId, this);
		setMouseCursor(juce::MouseCursor::CrosshairCursor);
	}
}

juce::Point<float> Macro::getDragSource()
{
	// drawHandle is a 14x14 shape; its centre sits at +(7, 7) from the
	// bounds passed into UIUtils::drawHandle in paint().
	constexpr float c = 7.f;
	return { 3.f + c, 3.f + c };
}

static int getStringWidth(const juce::Font& font, const juce::String& text)
{
	juce::GlyphArrangement ga;
	ga.addLineOfText(font, text, 0, 0);
	return (int)ga.getBoundingBox(0, -1, true).getWidth();
}

void Macro::paint(juce::Graphics& g)
{
	// draw background
	auto bounds = getLocalBounds().toFloat().reduced(0.5f);
	g.setColour(Colour(0xff333333));
	juce::Path p;
	p.addRoundedRectangle(bounds.getX(), bounds.getY(), lpad, bounds.getHeight(), 2.f, 2.f, true, false, true, false);
	g.fillPath(p);

	// draw outline
	g.setColour(Colours::black.withAlpha(0.25f));
	g.drawVerticalLine((int)(bounds.getX() + lpad), bounds.getY(), bounds.getBottom());
	g.setColour(juce::Colour(selected
		? COLOR_ACTIVE().withMultipliedBrightness(1.f)
		: Colours::black.withAlpha(0.25f)));
	g.drawRoundedRectangle(bounds, 2.f, 1.f);

	// draw handle
	UIUtils::drawHandle(g, bounds
		.withTrimmedRight(bounds.getWidth() - lpad)
		.withHeight(16.f).translated(3.f, 3.f), COLOR_VIEWPORT_TEXT());

	// draw connections number
	g.setFont(juce::FontOptions(10.f));
	g.setColour(juce::Colours::white.withAlpha(0.5f));
	g.drawFittedText(juce::String(connections), bounds
		.withTrimmedRight(bounds.getWidth() - lpad)
		.withY(20.f)
		.withHeight(12.f).toNearestInt(),
		juce::Justification::centred, 1);

	// draw text
	float fontSize = 14.0f;
	juce::String text = macroName;
	auto area = nameBtn.getBounds();
	juce::Font font(juce::FontOptions(13.f));

	// Shrink until it fits
	while (getStringWidth(font, text) > area.getWidth() && fontSize > 10.f) {
		fontSize -= 0.5f;
		font.setHeight(fontSize);
	}

	g.setFont(font);
	g.setColour(connections ? COLOR_VIEWPORT_TEXT() : (COLOR_VIEWPORT_TEXT()).withAlpha(0.3f));
	g.drawText(text, (int)area.getX(), (int)area.getY(), (int)area.getWidth(), (int)area.getHeight(), juce::Justification::centred, false);
}



void Macro::resized()
{
	auto bounds = getLocalBounds();

	rotary->setBounds(bounds.withTrimmedLeft((int)lpad));
	nameBtn.setBounds(rotary->getBounds().withHeight(20).withBottomY(rotary->getBottom()));
}

#include "Macro.h"
#include "../../PluginEditor.h"

Macro::Macro(RipplerAudioProcessorEditor& e, int index)
	: index(index)
    , editor(e)
    , theme(e.theme)
	, macroId("macro" + juce::String(index+1))
    , macroIdx(index)
{
	rotary = std::make_unique<Rotary>(editor, macroId, "Macro" + juce::String(index + 1), Rotary::Percent);
	rotary->setName ("rotary");
	addAndMakeVisible(rotary.get());
	rotary->radius = rotary->radius * 0.99f; // force image to re-render using the new rotary width/height
	rotary->yoffset -= 3;
	rotary->labelSize = 13.f;
	rotary->drawValue = false;
	rotary->drawBase = false;
	rotary->drawTextLabel = false;
	rotary->mod_offset = 3.f;
	rotary->mod_value_offset = 3.f;

	nameBtn.setName ("nameBtn");
	addAndMakeVisible(nameBtn);
	nameBtn.setAlpha(0.f);
	nameBtn.onClick = [this]
		{
            editor.audioProcessor.undomgr->createUndo();
			editor.showMacroRename(this);
		};

	startTimerHz(30);
	editor.registerModParam(rotary.get(), RipplerAudioProcessorEditor::kMacro);
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
	g.setColour(theme.COLOR_SHADE_MID());
	juce::Path p;
	p.addRoundedRectangle(bounds.getX(), bounds.getY(), lpad, bounds.getHeight(), 2.f, 2.f, true, false, true, false);
	g.fillPath(p);

	if (selected && theme.IS_LIGHT_THEME) {
		g.setColour(theme.COLOR_ACTIVE().withAlpha(0.5f));
		g.fillRoundedRectangle(bounds, 2.f);
	}

	// draw outline
	g.setColour(theme.COLOR_SHADE_HIGH().withAlpha(0.5f));
	g.drawVerticalLine((int)(bounds.getX() + lpad), bounds.getY(), bounds.getBottom());
	g.setColour(juce::Colour(selected
		? theme.COLOR_ACTIVE().withMultipliedBrightness(theme.IS_LIGHT_THEME ? 0.5f : 1.f)
		: theme.COLOR_SHADE_HIGH()));
	g.drawRoundedRectangle(bounds, 2.f, 1.f);

	// draw handle
	UIUtils::drawHandle(g, bounds
		.withTrimmedRight(bounds.getWidth() - lpad)
		.withHeight(16.f).translated(3.f, 3.f), theme.COLOR_TEXT_BRIGHT());

	// draw connections number
	g.setFont(juce::FontOptions(10.f));
	g.setColour(theme.COLOR_TEXT_DIM());
	g.drawFittedText(juce::String(connections), bounds
		.withTrimmedRight(bounds.getWidth() - lpad)
		.withY(20.f)
		.withHeight(12.f).toNearestInt(),
		juce::Justification::centred, 1);

	// draw text
	float fontSize = 13.0f;
	juce::String text = macroName;
	auto area = nameBtn.getBounds();
	juce::Font font(juce::FontOptions(13.f));

	// Shrink until it fits
	while (getStringWidth(font, text) > area.getWidth() && fontSize > 10.f) {
		fontSize -= 0.5f;
		font.setHeight(fontSize);
	}

	g.setFont(font);
	g.setColour(connections ? theme.COLOR_TEXT_BRIGHT() : (theme.COLOR_TEXT_BRIGHT()).withAlpha(theme.IS_LIGHT_THEME ? .6f : 0.3f));
	g.drawText(text, (int)area.getX(), (int)area.getY(), (int)area.getWidth(), (int)area.getHeight(), juce::Justification::centred, false);
}



void Macro::resized()
{
}

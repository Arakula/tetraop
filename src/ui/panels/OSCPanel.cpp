#include "OSCPanel.h"
#include "../../PluginEditor.h"

OSCPanel::OSCPanel(TetraOPAudioProcessorEditor& e, int _oscId)
	: editor(e)
	, oscId(_oscId)
	, prefix(_oscId == 0 ? "a_" : _oscId == 1 ? "b_" : _oscId == 2 ? "c_" : "d_")
{
	level = std::make_unique<Rotary>(e, prefix + "level", "Level", Rotary::gain2dB);
	pan = std::make_unique<Rotary>(e, prefix + "pan", "Pan", Rotary::Pan, true);
	phase = std::make_unique<Rotary>(editor, prefix + "phase_start", "Phase", Rotary::float2);
	rand = std::make_unique<Rotary>(editor, prefix + "phase_rand", "Rand", Rotary::Percent);
	morph = std::make_unique<Rotary>(editor, prefix + "morph", "Frame", Rotary::float3);
	dist = std::make_unique<Rotary>(editor, prefix + "phase_dist", "Dist", Rotary::Percent);
	detune = std::make_unique<Rotary>(editor, prefix + "unison_detune", "Det", Rotary::Percent);
	blend = std::make_unique<Rotary>(editor, prefix + "unison_blend", "Blend", Rotary::Percent);
	wide = std::make_unique<Rotary>(editor, prefix + "unison_stereo", "Wide", Rotary::Percent);
	semis = std::make_unique<Rotary>(editor, prefix + "pitch_semis", "Semis", Rotary::PitchSemis, true);
	cents = std::make_unique<Rotary>(editor, prefix + "pitch_cents", "Fine", Rotary::Integer, true);

	detune->setSmall();
	blend->setSmall();
	wide->setSmall();

	editor.registerModParam(level.get());
	editor.registerModParam(pan.get());
	editor.registerModParam(phase.get());
	editor.registerModParam(rand.get());
	editor.registerModParam(morph.get());
	editor.registerModParam(dist.get());
	editor.registerModParam(detune.get());
	editor.registerModParam(blend.get());
	editor.registerModParam(wide.get());
	editor.registerModParam(semis.get());
	editor.registerModParam(cents.get());

	addAndMakeVisible(level.get());
	addAndMakeVisible(pan.get());
	addAndMakeVisible(phase.get());
	addAndMakeVisible(rand.get());
	addAndMakeVisible(morph.get());
	addAndMakeVisible(dist.get());
	addAndMakeVisible(detune.get());
	addAndMakeVisible(blend.get());
	addAndMakeVisible(wide.get());
	addAndMakeVisible(semis.get());
	addAndMakeVisible(cents.get());

	
}

OSCPanel::~OSCPanel()
{

}

void OSCPanel::parameterChanged(const juce::String& parameterID, float newValue)
{

}

void OSCPanel::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();
	UIUtils::drawPanel(g, b, true);
	auto viewport = Rectangle<int>(KNOB_WIDTH * 2, PANEL_HEADER_HEIGHT + 5, KNOB_WIDTH_SM * 3, KNOB_HEIGHT + 8)
		.toFloat().translated(0.5f, 0.5f).reduced(2.f, 0.f);
	UIUtils::drawBevel(g, viewport, 5.f, Colours::black);

	g.setColour(COLOR_BACKGROUND());
	g.fillRoundedRectangle(viewport.reduced(1).withTrimmedTop(1).withTrimmedLeft(1), 5.f);

	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(FontOptions(16.f));
	g.drawText("Unison", viewport.getX(), viewport.getBottom() - 18 - 2, 60, 18, Justification::centred);
	g.setColour(Colours::white.withAlpha(0.20f));
	g.drawVerticalLine((int)viewport.getX() + 60, viewport.getBottom() - 18, viewport.getBottom() - 3);
	g.drawVerticalLine((int)viewport.getX() + 60 + 30, viewport.getBottom() - 18, viewport.getBottom() - 3);
	g.setColour(COLOR_KNOB_LABEL());
	g.drawText("16", viewport.getX() + 60, viewport.getBottom() - 18-2, 30, 18, Justification::centred);
}

void OSCPanel::resized()
{
	auto bounds = getLocalBounds();
	bounds.translate(0, PANEL_HEADER_HEIGHT);
	level->setBounds(bounds.getX(), bounds.getY(), KNOB_WIDTH, KNOB_HEIGHT);
	pan->setBounds(level->getBounds().translated(KNOB_WIDTH, 0));
	phase->setBounds(level->getBounds().translated(0, KNOB_HEIGHT));
	rand->setBounds(level->getBounds().translated(KNOB_WIDTH, KNOB_HEIGHT));
	morph->setBounds(pan->getBounds().translated(KNOB_WIDTH_SM * 3 + KNOB_WIDTH, 0));
	dist->setBounds(morph->getBounds().translated(KNOB_WIDTH, 0));
	semis->setBounds(morph->getBounds().translated(0, KNOB_HEIGHT));
	cents->setBounds(morph->getBounds().translated(KNOB_WIDTH, KNOB_HEIGHT));
	detune->setBounds(rand->getBounds()
		.withWidth(KNOB_WIDTH_SM)
		.withHeight(KNOB_HEIGHT_SM)
		.withBottomY(rand->getBottom()).translated(KNOB_WIDTH, 0));
	blend->setBounds(detune->getBounds().translated(KNOB_WIDTH_SM, 0));
	wide->setBounds(blend->getBounds().translated(KNOB_WIDTH_SM, 0));
}

void OSCPanel::toggleUIComponents()
{

}
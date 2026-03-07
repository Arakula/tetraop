#include "OSCPanel.h"
#include "../../PluginEditor.h"

OSCPanel::OSCPanel(TetraOPAudioProcessorEditor& e, int _oscId)
	: editor(e)
	, oscId(_oscId)
	, prefix(_oscId == 0 ? "a_" : _oscId == 1 ? "b_" : _oscId == 2 ? "c_" : "d_")
{
	editor.audioProcessor.params.addParameterListener(prefix + "on", this);

	addAndMakeVisible(onBtn);
	onBtn.setAlpha(0.f);
	onBtn.onClick = [this]
		{
			auto param = editor.audioProcessor.params.getParameter(prefix + "on");
			param->setValueNotifyingHost(param->getValue() > 0.f ? 0.f : 1.f);
		};

	level = std::make_unique<Rotary>(e, prefix + "level", "Level", Rotary::gain2dB);
	pan = std::make_unique<Rotary>(e, prefix + "pan", "Pan", Rotary::Pan, true);
	phase = std::make_unique<Rotary>(editor, prefix + "phase_offset", "Phase", Rotary::float2);
	rand = std::make_unique<Rotary>(editor, prefix + "phase_rand", "Rand", Rotary::Percent);
	auto rotaryFormat = oscId == 0 ? Rotary::OSCMorphA : oscId == 1 ? Rotary::OSCMorphB : oscId == 2 ? Rotary::OSCMorphC : Rotary::OSCMorphD;
	morph = std::make_unique<Rotary>(editor, prefix + "morph", "Frame", rotaryFormat);
	dist = std::make_unique<Rotary>(editor, prefix + "phase_dist", "Dist", Rotary::Percent);
	detune = std::make_unique<Rotary>(editor, prefix + "unison_detune", "Det", Rotary::Percent);
	blend = std::make_unique<Rotary>(editor, prefix + "unison_blend", "Blend", Rotary::Percent);
	wide = std::make_unique<Rotary>(editor, prefix + "unison_stereo", "Wide", Rotary::Percent);
	semis = std::make_unique<Rotary>(editor, prefix + "pitch_semis", "Semis", Rotary::PitchSemis, true);
	cents = std::make_unique<Rotary>(editor, prefix + "pitch_cents", "Fine", Rotary::Integer, true);

	morph->onMouseDown = [this] { onMouseDownMorph(); };
	morph->onMouseUp = [this] { onMouseUpMorph(); };

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

	unison = std::make_unique<UnisonWidget>(editor, oscId);
	addAndMakeVisible(unison.get());

	waveDisplay = std::make_unique<WaveDisplay>(editor, oscId);
	addAndMakeVisible(waveDisplay.get());
}

OSCPanel::~OSCPanel()
{
	editor.audioProcessor.params.removeParameterListener(prefix + "on", this);
}

void OSCPanel::parameterChanged(const juce::String& paramId, float val)
{
	(void)paramId;
	(void)val;
	MessageManager::callAsync([this] { repaint(); });
}

void OSCPanel::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();
	UIUtils::drawPanel(g, b, true);
	UIUtils::drawBevel(g, viewport, 5.f, Colours::black);

	g.setColour(COLOR_BACKGROUND());
	g.fillRoundedRectangle(viewport.reduced(1).withTrimmedTop(1).withTrimmedLeft(1), 5.f);

	auto headerb = b.withHeight(PANEL_HEADER_HEIGHT);
	bool on = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "on")->load();
	auto c = oscId == 0 ? COLOR_A() : oscId == 1 ? COLOR_B() : oscId == 2 ? COLOR_C() : COLOR_D();
	UIUtils::drawCheckmark(g, onBtn.getBounds().toFloat(), COLOR_CHECKMARK_BG_LIGHT(), c, on);
	g.setColour(COLOR_PANEL_HEADER_TEXT());
	g.saveState();
	g.setFont(editor.customLookAndFeel->getBoldFont(16.f));
	auto name = prefix.substring(0,1).toUpperCase();
	g.drawText(name, headerb.withWidth(20).translated(20, 0), Justification::centred);
	g.restoreState();
}

void OSCPanel::resized()
{
	auto bounds = getLocalBounds();
	onBtn.setBounds({ bounds.getX(), bounds.getY(), PANEL_HEADER_HEIGHT, PANEL_HEADER_HEIGHT});

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

	viewport = Rectangle<int>(KNOB_WIDTH * 2, PANEL_HEADER_HEIGHT + 5, KNOB_WIDTH_SM * 3, KNOB_HEIGHT + 8)
		.toFloat().translated(0.5f, 0.5f).reduced(2.f, 0.f);

	unison->setBounds(viewport.toNearestInt().removeFromBottom(20));
	waveDisplay->setBounds(viewport.withTrimmedBottom(20.f).reduced(2.f).toNearestInt());
}

void OSCPanel::onMouseDownMorph() const
{
	waveDisplay->isMorphing = true;
}

void OSCPanel::onMouseUpMorph() const
{
	waveDisplay->isMorphing = false;
}

void OSCPanel::toggleUIComponents()
{

}
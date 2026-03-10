#include "OSCPanel.h"
#include "../../PluginEditor.h"

OSCPanel::OSCPanel(TetraOPAudioProcessorEditor& e, int _oscId)
	: editor(e)
	, oscId(_oscId)
	, prefix(_oscId == 0 ? "a_" : _oscId == 1 ? "b_" : _oscId == 2 ? "c_" : "d_")
{
	editor.audioProcessor.params.addParameterListener(prefix + "on", this);
	editor.audioProcessor.params.addParameterListener(prefix + "phase_dist_mode", this);

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
	dist = std::make_unique<Rotary>(editor, prefix + "phase_dist_amt", "", Rotary::Percent, true);
	detune = std::make_unique<Rotary>(editor, prefix + "unison_detune", "Det", Rotary::Percent);
	blend = std::make_unique<Rotary>(editor, prefix + "unison_blend", "Blend", Rotary::Percent);
	wide = std::make_unique<Rotary>(editor, prefix + "unison_stereo", "Wide", Rotary::Percent);
	semis = std::make_unique<Rotary>(editor, prefix + "pitch_semis", "Semis", Rotary::PitchSemis, true);
	cents = std::make_unique<Rotary>(editor, prefix + "pitch_cents", "Fine", Rotary::Integer, true);

	morph->onMouseDown = [this] { onMouseDownMorph(); };
	morph->onMouseUp = [this] { onMouseUpMorph(); };

	dist->onMouseDown = [this] { isMouseDownDist = true; repaint(); };
	dist->onMouseUp = [this] { isMouseDownDist = false; repaint(); };

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

	addAndMakeVisible(distBtn);
	distBtn.setAlpha(0.f);
	distBtn.onClick = [this] { showDistortionMenu(); };

	unison = std::make_unique<UnisonWidget>(editor, oscId);
	addAndMakeVisible(unison.get());

	waveDisplay = std::make_unique<WaveDisplay>(editor, oscId);
	addAndMakeVisible(waveDisplay.get());

	toggleUIComponents();
}

OSCPanel::~OSCPanel()
{
	editor.audioProcessor.params.removeParameterListener(prefix + "on", this);
	editor.audioProcessor.params.removeParameterListener(prefix + "phase_dist_mode", this);
}

void OSCPanel::parameterChanged(const juce::String& paramId, float val)
{
	(void)paramId;
	(void)val;
	toggleUIComponents();
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

	g.setFont(FontOptions(16.f));
	g.setColour(Colours::black.withAlpha(0.15f));
	g.fillRoundedRectangle(distBtn.getBounds().toFloat(), 3.f);

	auto distMode = (PhaseDist::Mode)editor.audioProcessor.params.getRawParameterValue(prefix + "phase_dist_mode")->load();
	auto text = "Off";
	switch (distMode)
	{
	case PhaseDist::Bend: text = "Bend"; break;
	case PhaseDist::Bias: text = "Bias"; break;
	case PhaseDist::Fold: text = "Fold"; break;
	case PhaseDist::Formant: text = "Formt"; break;
	case PhaseDist::Pulse: text = "Pulse"; break;
	case PhaseDist::Quantize: text = "Qnt"; break;
	case PhaseDist::Skew: text = "Skew"; break;
	case PhaseDist::Sync: text = "Sync"; break;
	}

	if (!isMouseDownDist)
	{
		g.setColour(COLOR_KNOB_LABEL());
		g.drawText(text, distBtn.getBounds().toFloat(), Justification::centred);
	}
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

	distBtn.setBounds(dist->getBounds().removeFromBottom(20).reduced(4, 0));

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
	bool on = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "on")->load();
	level->disabled = !on;
	level->disabled = !on;
	pan->disabled = !on;
	phase->disabled = !on;
	rand->disabled = !on;
	morph->disabled = !on;
	dist->disabled = !on;
	detune->disabled = !on;
	blend->disabled = !on;
	wide->disabled = !on;
	semis->disabled = !on;
	cents->disabled = !on;

	MessageManager::callAsync([this] { repaint(); });
}

void OSCPanel::showDistortionMenu()
{
	auto mode = (PhaseDist::Mode)editor.audioProcessor.params.getRawParameterValue(prefix + "phase_dist_mode")->load();

	PopupMenu menu;
	menu.addItem(1, "Off", true, mode == PhaseDist::Off);
	menu.addItem(2, "Bend", true, mode == PhaseDist::Bend);
	menu.addItem(3, "Skew", true, mode == PhaseDist::Skew);
	menu.addItem(4, "Bias", true, mode == PhaseDist::Bias);
	menu.addItem(5, "Pulse", true, mode == PhaseDist::Pulse);
	menu.addItem(6, "Sync", true, mode == PhaseDist::Sync);
	menu.addItem(7, "Formant", true, mode == PhaseDist::Formant);
	menu.addItem(8, "Quantize", true, mode == PhaseDist::Quantize);
	menu.addItem(9, "Fold", true, mode == PhaseDist::Fold);

	auto menuPos = localPointToGlobal(distBtn.getBounds().getBottomLeft());
	menu.showMenuAsync(PopupMenu::Options()
		.withTargetComponent(*this)
		.withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
		[this](int result) {
			if (result == 0) return;

			auto param = editor.audioProcessor.params.getParameter(prefix + "phase_dist_mode");
			param->setValueNotifyingHost(param->convertTo0to1((float)(result - 1)));
		});
}
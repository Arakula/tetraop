#include "OSCPanel.h"
#include "../../PluginEditor.h"

OSCPanel::OSCPanel(TetraOPAudioProcessorEditor& e, int _oscId)
	: editor(e)
	, oscId(_oscId)
	, prefix(_oscId == 0 ? "a_" : _oscId == 1 ? "b_" : _oscId == 2 ? "c_" : "d_")
{
	startTimerHz(10);

	editor.audioProcessor.params.addParameterListener(prefix + "on", this);
	editor.audioProcessor.params.addParameterListener(prefix + "phase_dist_mode", this);
	editor.audioProcessor.params.addParameterListener(prefix + "morph_snap", this);

	addAndMakeVisible(onBtn);
	onBtn.setAlpha(0.f);
	onBtn.onClick = [this]
		{
			editor.audioProcessor.undomgr->createUndo();
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

	auto cat = oscId == 0 ? TetraOPAudioProcessorEditor::kOSCA
		: oscId == 1 ? TetraOPAudioProcessorEditor::kOSCB
		: oscId == 2 ? TetraOPAudioProcessorEditor::kOSCC
		: TetraOPAudioProcessorEditor::kOSCD;

	editor.registerModParam(level.get(), cat);
	editor.registerModParam(pan.get(), cat);
	editor.registerModParam(phase.get(), cat);
	editor.registerModParam(rand.get(), cat);
	editor.registerModParam(morph.get(), cat);
	editor.registerModParam(dist.get(), cat);
	editor.registerModParam(detune.get(), cat);
	editor.registerModParam(blend.get(), cat);
	editor.registerModParam(wide.get(), cat);
	editor.registerModParam(semis.get(), cat);
	editor.registerModParam(cents.get(), cat);

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

	addAndMakeVisible(morphBtn);
	morphBtn.setAlpha(0.f);
	morphBtn.onClick = [this] 
		{
			auto param = editor.audioProcessor.params.getParameter(prefix + "morph_snap");
			param->setValueNotifyingHost(param->getValue() > 0.f ? 0.f : 1.f);
		};

	unison = std::make_unique<UnisonWidget>(editor, oscId);
	addAndMakeVisible(unison.get());

	waveDisplay = std::make_unique<WaveDisplay>(editor, oscId);
	addAndMakeVisible(waveDisplay.get());

	addAndMakeVisible(tableBtn);
	tableBtn.setAlpha(0.f);
	tableBtn.onClick = [this] { showWavetablesMenu(); };

	addAndMakeVisible(nextBtn);
	nextBtn.setAlpha(0.f);
	nextBtn.onClick = [this]
		{
			editor.audioProcessor.tablesMgr->loadNext(oscId);
		};

	addAndMakeVisible(prevBtn);
	prevBtn.setAlpha(0.f);
	prevBtn.onClick = [this]
		{
			editor.audioProcessor.tablesMgr->loadPrev(oscId);
		};

	feedback = std::make_unique<ValuePicker>(editor, prefix + "feedback");
	feedback->isPercentage = true;
	feedback->suffix = "%";
	//feedback->align = Justification::centredRight;
	feedback->color = COLOR_PANEL_HEADER_TEXT().withAlpha(0.5f);
	feedback->precision = 0;
	feedback->fontSize = 14.f;
	addAndMakeVisible(feedback.get());

	octave = std::make_unique<ValuePicker>(editor, prefix + "pitch_oct");
	octave->color = COLOR_KNOB_LABEL();
	octave->precision = 0;
	octave->fontSize = 14.f;
	octave->prefix = "Oct";
	addAndMakeVisible(octave.get());

	toggleUIComponents();
}

OSCPanel::~OSCPanel()
{
	editor.audioProcessor.params.removeParameterListener(prefix + "on", this);
	editor.audioProcessor.params.removeParameterListener(prefix + "phase_dist_mode", this);
	editor.audioProcessor.params.removeParameterListener(prefix + "morph_snap", this);
}

void OSCPanel::parameterChanged(const juce::String& paramId, float val)
{
	(void)paramId;
	(void)val;
	toggleUIComponents();
}

void OSCPanel::timerCallback()
{
	if (editor.audioProcessor.tablesMgr->UIDirty[oscId].exchange(false))
	{
		toggleUIComponents();
		waveDisplay->toggleUIComponents();
	}
}

void OSCPanel::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();
	auto c = oscId == 0 ? COLOR_A() : oscId == 1 ? COLOR_B() : oscId == 2 ? COLOR_C() : COLOR_D();
	bool on = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "on")->load();

	UIUtils::drawPanel(g, b, true, false, COLOR_PANEL_HEADER());
	UIUtils::drawBevel(g, viewport, 5.f, Colours::black);

	g.setColour(COLOR_BACKGROUND());
	g.fillRoundedRectangle(viewport.reduced(1).withTrimmedTop(1).withTrimmedLeft(1), 5.f);

	auto headerb = b.withHeight(PANEL_HEADER_HEIGHT);
	g.setColour(COLOR_PANEL_HEADER_TEXT());
	auto& tableName = editor.audioProcessor.tablesMgr->wavetables[oscId].name;
	g.setFont(FontOptions(16.f));
	g.drawText(tableName, tableBtn.getBounds(), Justification::centred);
	UIUtils::drawTriangle(g, prevBtn.getBounds().toFloat().reduced(7.f), 3, COLOR_PANEL_HEADER_TEXT());
	UIUtils::drawTriangle(g, nextBtn.getBounds().toFloat().reduced(7.f), 1, COLOR_PANEL_HEADER_TEXT());

	UIUtils::drawCheckmark(g, onBtn.getBounds().toFloat(), COLOR_CHECKMARK_BG_LIGHT(), c, on);
	g.setColour(COLOR_PANEL_HEADER_TEXT());
	g.saveState();
	g.setFont(editor.customLookAndFeel->getBoldFont(16.f));
	auto name = prefix.substring(0,1).toUpperCase();
	g.drawText(name, headerb.withWidth(20).translated(20, 0), Justification::centred);
	g.restoreState();

	auto distMode = (PhaseDist::Mode)editor.audioProcessor.params.getRawParameterValue(prefix + "phase_dist_mode")->load();
	g.setFont(FontOptions(16.f));
	g.setColour(Colours::black.withAlpha(0.15f));
	g.fillRoundedRectangle(distBtn.getBounds().toFloat().translated(0.5f,0.5f), 3.f);
	if (distMode != PhaseDist::Off)
	{
		g.setColour(COLOR_ACTIVE().withAlpha(0.5f));
		g.fillRoundedRectangle(distBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f);
	}
	g.setColour(Colours::black.withAlpha(0.35f));
	g.drawRoundedRectangle(distBtn.getBounds().toFloat().translated(0.5f,0.5f), 3.f, 1.f);

	bool morphSnap = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "morph_snap")->load();
	g.setColour(Colours::black.withAlpha(0.15f));
	g.fillRoundedRectangle(morphBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f);
	if (!morphSnap)
	{
		g.setColour(COLOR_ACTIVE().withAlpha(0.5f));
		g.fillRoundedRectangle(morphBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f);
	}
	g.setColour(Colours::black.withAlpha(0.35f));
	g.drawRoundedRectangle(morphBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f, 1.f);

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

	UIUtils::drawFeedback(g, feedback->getBounds().withWidth(PANEL_HEADER_HEIGHT)
		.withX(feedback->getX() -15).reduced(5, 5).toFloat(), COLOR_PANEL_HEADER_TEXT().withAlpha(0.5f));

	auto r = octave->getBounds().reduced(8, 4).translated(-13, 0).toFloat().translated(0.5f, 0.5f);
	g.setColour(Colours::black.brighter(0.5f));
	g.fillRoundedRectangle(r, 3.f);
}

void OSCPanel::resized()
{
	auto bounds = getLocalBounds();
	auto headerb = bounds.withHeight(PANEL_HEADER_HEIGHT);

	tableBtn.setBounds(Rectangle<int>{ KNOB_WIDTH_SM * 3, headerb.getHeight()}
		.translated(headerb.getCentreX() - KNOB_WIDTH_SM * 3 / 2, 0));

	nextBtn.setBounds(Rectangle<int>(PANEL_HEADER_HEIGHT, PANEL_HEADER_HEIGHT)
		.withX(tableBtn.getRight()));

	prevBtn.setBounds(Rectangle<int>(PANEL_HEADER_HEIGHT, PANEL_HEADER_HEIGHT)
		.withRightX(tableBtn.getX()));

	octave->setBounds(nextBtn.getBounds().withWidth(60).translated(30, 0));

	onBtn.setBounds({ bounds.getX(), bounds.getY(), PANEL_HEADER_HEIGHT, PANEL_HEADER_HEIGHT});
	feedback->setBounds(onBtn.getBounds().translated(50, 0).withWidth(40));
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
	morphBtn.setBounds(morph->getBounds().removeFromBottom(20).reduced(4, 0));

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
	bool morphSnap = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "morph_snap")->load();
	bool on = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "on")->load();
	level->setEnabled(on);
	pan->setEnabled(on);
	phase->setEnabled(on);
	rand->setEnabled(on);
	morph->setEnabled(on);
	dist->setEnabled(on);
	detune->setEnabled(on);
	blend->setEnabled(on);
	wide->setEnabled(on);
	semis->setEnabled(on);
	cents->setEnabled(on);

	if (!morphSnap)
	{
		morph->format = oscId == 0 ? Rotary::OSCMorphA2f : oscId == 1 ? Rotary::OSCMorphB2f
			: oscId == 1 ? Rotary::OSCMorphC2f : Rotary::OSCMorphD2f;
		morph->name = "Morph";
	}
	else
	{
		morph->format = oscId == 0 ? Rotary::OSCMorphA : oscId == 1 ? Rotary::OSCMorphB
			: oscId == 1 ? Rotary::OSCMorphC : Rotary::OSCMorphD;
		morph->name = "Frame";
	}

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

static void buildWavetablesMenu(PopupMenu& menu, const std::vector<TablesManager::TableFolder>& folders, int selected)
{
	for (auto& folder : folders)
	{
		PopupMenu sub;

		if (!folder.children.empty()) 
		{
			buildWavetablesMenu(sub, folder.children, selected);
		}

		if (!folder.files.empty())
		{

			for (auto& file : folder.files)
			{
				if (sub.getNumItems() > 0 && sub.getNumItems() % 20 == 0)
					sub.addColumnBreak();

				sub.addItem(file.id + 4, file.name, true, selected == file.id);
			}
		}

		menu.addSubMenu(folder.name, sub);
	}
}

void OSCPanel::showWavetablesMenu()
{
	auto& table = editor.audioProcessor.tablesMgr->wavetables[oscId];

	PopupMenu menu;
	menu.addItem(1, "Basic Shapes", true, table.mode == TablesManager::BasicShapes);
	menu.addItem(2, "White Noise", true, table.mode == TablesManager::WhiteNoise);
	menu.addItem(3, "Pink Noise", true, table.mode == TablesManager::PinkNoise);
	menu.addSeparator();

	auto roots = editor.audioProcessor.tablesMgr->getFolderTree();

	buildWavetablesMenu(menu, roots, table.fileId);

	menu.addSeparator();
	menu.addItem(99999, "Open Folder");

	auto menuPos = localPointToGlobal(tableBtn.getBounds().getBottomLeft());
	menu.showMenuAsync(PopupMenu::Options()
		.withTargetComponent(*this)
		.withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
		[this](int result) {
			if (result == 0) return;

			if (result == 99999)
			{
				auto dir = File(editor.audioProcessor.tablesMgr->dir);
				dir.startAsProcess();
				return;
			}

			editor.audioProcessor.tablesMgr->loadFromId(oscId, result - 4);
		});
}


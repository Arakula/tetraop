#include "FilterPanel.h"
#include "../../PluginEditor.h"

FilterPanel::FilterPanel(TetraOPAudioProcessorEditor& e, int _fid)
	: editor(e)
	, fid(_fid)
	, prefix(_fid == 0 ? "f1_" : "f2_")
{
	editor.audioProcessor.params.addParameterListener(prefix + "on", this);
	editor.audioProcessor.params.addParameterListener(prefix + "type", this);
	editor.audioProcessor.params.addParameterListener(prefix + "mode", this);
	editor.audioProcessor.params.addParameterListener(prefix + "inA", this);
	editor.audioProcessor.params.addParameterListener(prefix + "inB", this);
	editor.audioProcessor.params.addParameterListener(prefix + "inC", this);
	editor.audioProcessor.params.addParameterListener(prefix + "inD", this);
	if (fid > 0)
		editor.audioProcessor.params.addParameterListener(prefix + "inF1", this);

	addAndMakeVisible(onBtn);
	onBtn.setAlpha(0.f);
	onBtn.onClick = [this]
		{
			auto param = editor.audioProcessor.params.getParameter(prefix + "on");
			param->setValueNotifyingHost(param->getValue() > 0.f ? 0.f : 1.f);
		};

	cut = std::make_unique<Rotary>(e, prefix + "cut", "Cut", Rotary::Hz);
	res = std::make_unique<Rotary>(e, prefix + "res", "Res", Rotary::Percent);
	drive = std::make_unique<Rotary>(e, prefix + "drive", "Drive", Rotary::Percent);
	mix = std::make_unique<Rotary>(e, prefix + "mix", "Mix", Rotary::Percent);

	editor.registerModParam(cut.get());
	editor.registerModParam(res.get());
	editor.registerModParam(drive.get());
	editor.registerModParam(mix.get());

	addAndMakeVisible(cut.get());
	addAndMakeVisible(res.get());
	addAndMakeVisible(drive.get());
	addAndMakeVisible(mix.get());

	addAndMakeVisible(modeBtn);
	modeBtn.setAlpha(0.f);
	modeBtn.onClick = [this] { showModeMenu(); };

	addAndMakeVisible(typeBtn);
	typeBtn.setAlpha(0.f);
	typeBtn.onClick = [this] { showTypeMenu(); };

	addAndMakeVisible(inaBtn); 
	inaBtn.setAlpha(0.f);
	addAndMakeVisible(inbBtn);
	inbBtn.setAlpha(0.f);
	addAndMakeVisible(incBtn);
	incBtn.setAlpha(0.f);
	addAndMakeVisible(indBtn);
	indBtn.setAlpha(0.f);
	if (fid > 0) 
	{
		addAndMakeVisible(inf1Btn);
		inf1Btn.setAlpha(0.f);
	}

	toggleUIComponents();
}

FilterPanel::~FilterPanel()
{
	editor.audioProcessor.params.removeParameterListener(prefix + "on", this);
	editor.audioProcessor.params.removeParameterListener(prefix + "type", this);
	editor.audioProcessor.params.removeParameterListener(prefix + "mode", this);
	editor.audioProcessor.params.removeParameterListener(prefix + "inA", this);
	editor.audioProcessor.params.removeParameterListener(prefix + "inB", this);
	editor.audioProcessor.params.removeParameterListener(prefix + "inC", this);
	editor.audioProcessor.params.removeParameterListener(prefix + "inD", this);
	if (fid > 0)
		editor.audioProcessor.params.removeParameterListener(prefix + "inF1", this);
}

void FilterPanel::parameterChanged(const juce::String& paramId, float val)
{
	(void)paramId;
	(void)val;
	toggleUIComponents();
}

void FilterPanel::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();
	UIUtils::drawPanel(g, b, true);

	UIUtils::drawBevel(g, typeBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f, COLOR_BEVEL());
	UIUtils::drawBevel(g, modeBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f, COLOR_BEVEL());

	auto type = (Filter::Type)editor.audioProcessor.params.getRawParameterValue(prefix + "type")->load();
	auto mode = (Filter::Mode)editor.audioProcessor.params.getRawParameterValue(prefix + "mode")->load();
	bool on = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "on")->load();
	bool ina = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "inA")->load();
	bool inb = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "inB")->load();
	bool inc = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "inC")->load();
	bool ind = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "inD")->load();

	auto headerb = b.withHeight(PANEL_HEADER_HEIGHT);
	UIUtils::drawCheckmark(g, onBtn.getBounds().toFloat(), COLOR_CHECKMARK_BG_LIGHT(), COLOR_ACTIVE(), on);
	UIUtils::drawCheckmark(g, inaBtn.getBounds().toFloat(), Colours::black.withAlpha(0.5f), COLOR_ACTIVE(), ina && on);
	UIUtils::drawCheckmark(g, inbBtn.getBounds().toFloat(), Colours::black.withAlpha(0.5f), COLOR_ACTIVE(), inb && on);
	UIUtils::drawCheckmark(g, incBtn.getBounds().toFloat(), Colours::black.withAlpha(0.5f), COLOR_ACTIVE(), inc && on);
	UIUtils::drawCheckmark(g, indBtn.getBounds().toFloat(), Colours::black.withAlpha(0.5f), COLOR_ACTIVE(), ind && on);
	if (fid > 0) 
	{
		bool inf1 = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "inF1")->load();
		UIUtils::drawCheckmark(g, inf1Btn.getBounds().toFloat(), Colours::black.withAlpha(0.5f), COLOR_ACTIVE(), inf1 && on);
	}

	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(FontOptions(16.f));
	g.drawText("A", inaBtn.getBounds().translated(17, 0).withWidth(40), Justification::centredLeft);
	g.drawText("B", inbBtn.getBounds().translated(17, 0).withWidth(40), Justification::centredLeft);
	g.drawText("C", incBtn.getBounds().translated(17, 0).withWidth(40), Justification::centredLeft);
	g.drawText("D", indBtn.getBounds().translated(17, 0).withWidth(40), Justification::centredLeft);
	if (fid > 0)
	{
		g.drawText("F1", inf1Btn.getBounds().translated(17, 0).withWidth(40), Justification::centredLeft);
	}

	g.setColour(COLOR_PANEL_HEADER_TEXT());
	g.drawText(fid == 0 ? "Filter 1" : "Filter 2", headerb.withWidth(50).translated(25, 0), Justification::centredLeft);

	auto text = "";
	switch (type)
	{
		case Filter::kDigital12 : text = "D12p"; break;
		case Filter::kDigital24: text = "D24p"; break;
		case Filter::kAnalog12: text = "A12p"; break;
		case Filter::kAnalog24: text = "A24p"; break;
		case Filter::kLadder12: text = "Lad12p"; break;
		case Filter::kLadder24: text = "Lad24p"; break;
		case Filter::kTB303 : text = "303"; break;
		case Filter::kPhaserPos : text = "Phs+"; break;
		case Filter::kPhaserNeg : text = "Phs-"; break;
	}

	g.setFont(FontOptions(16.f));
	g.setColour(COLOR_ACTIVE());
	g.drawText(text, typeBtn.getBounds(), Justification::centred);

	text = "";
	switch (mode)
	{
		case Filter::LP : text = "LP"; break;
		case Filter::HP : text = "HP"; break;
		case Filter::BP : text = "BP"; break;
		case Filter::BS : text = "BS"; break;
		case Filter::PK : text = "PK"; break;
	}

	if (type == Filter::kTB303 && (mode == Filter::BS || mode == Filter::PK))
		text = "LP";

	if (type == Filter::kPhaserNeg || type == Filter::kPhaserPos)
		text = "---";

	g.setColour(COLOR_ACTIVE());
	g.drawText(text, modeBtn.getBounds(), Justification::centred);
}

void FilterPanel::resized()
{
	auto bounds = getLocalBounds();
	onBtn.setBounds({ bounds.getX(), bounds.getY(), PANEL_HEADER_HEIGHT, PANEL_HEADER_HEIGHT});

	bounds.translate(0, PANEL_HEADER_HEIGHT);
	typeBtn.setBounds(Rectangle<int>{bounds.getX() + FILTER_PANEL_HMARGIN, bounds.getY(), KNOB_WIDTH + 5, 23}.translated(0, 18));
	modeBtn.setBounds(typeBtn.getBounds().translated(0, 28));

	cut->setBounds(bounds.getX() + KNOB_WIDTH + FILTER_PANEL_HMARGIN * 2, bounds.getY(), KNOB_WIDTH, KNOB_HEIGHT);
	res->setBounds(cut->getBounds().translated(KNOB_WIDTH, 0));
	drive->setBounds(cut->getBounds().translated(0, KNOB_HEIGHT));
	mix->setBounds(cut->getBounds().translated(KNOB_WIDTH, KNOB_HEIGHT));

	inaBtn.setBounds(Rectangle<int>{15,15}.translated(bounds.getX() + FILTER_PANEL_HMARGIN, bounds.getY() + KNOB_HEIGHT + (fid == 0 ? 15 : 30)));
	inbBtn.setBounds(inaBtn.getBounds().translated(35, 0));
	incBtn.setBounds(inaBtn.getBounds().translated(0, 22));
	indBtn.setBounds(inaBtn.getBounds().translated(35, 22));

	if (fid > 0)
		inf1Btn.setBounds(inaBtn.getBounds().translated(0, -22));
}


void FilterPanel::toggleUIComponents()
{
	bool on = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "on")->load();
	cut->disabled = !on;
	res->disabled = !on;
	drive->disabled = !on;
	mix->disabled = !on;
	MessageManager::callAsync([this] { repaint(); });
}

void FilterPanel::showTypeMenu()
{
	auto type = (Filter::Type)editor.audioProcessor.params.getRawParameterValue(prefix + "type")->load();

	PopupMenu menu;
	menu.addItem(1, "Digital 12p", true, type == Filter::kDigital12);
	menu.addItem(2, "Digital 24p", true, type == Filter::kDigital24);
	menu.addItem(3, "Analog 12p", true, type == Filter::kAnalog12);
	menu.addItem(4, "Analog 24p", true, type == Filter::kAnalog24);
	menu.addItem(5, "Ladder 12p", true, type == Filter::kLadder12);
	menu.addItem(6, "Ladder 24p", true, type == Filter::kLadder24);
	menu.addItem(7, "303", true, type == Filter::kTB303);
	menu.addItem(8, "Phaser+", true, type == Filter::kPhaserPos);
	menu.addItem(9, "Phaser-", true, type == Filter::kPhaserNeg);

	auto menuPos = localPointToGlobal(typeBtn.getBounds().getBottomLeft());
	menu.showMenuAsync(PopupMenu::Options()
		.withTargetComponent(*this)
		.withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
		[this](int result) {
			if (result == 0) return;

			auto param = editor.audioProcessor.params.getParameter(prefix + "type");
			param->setValueNotifyingHost(param->convertTo0to1((float)(result - 1)));
		});
}

void FilterPanel::showModeMenu()
{
	auto type = (Filter::Type)editor.audioProcessor.params.getRawParameterValue(prefix + "type")->load();
	if (type == Filter::kPhaserNeg || type == Filter::kPhaserPos)
		return;

	auto mode = (Filter::Mode)editor.audioProcessor.params.getRawParameterValue(prefix + "mode")->load();

	PopupMenu menu;
	menu.addItem(1, "LowPass", true, mode == Filter::LP);
	menu.addItem(2, "BandPass", true, mode == Filter::BP);
	menu.addItem(3, "HighPass", true, mode == Filter::HP);
	if (type != Filter::kTB303)
	{
		menu.addItem(4, "BandStop", true, mode == Filter::BS);
		menu.addItem(5, "Peak", true, mode == Filter::PK);
	}

	auto menuPos = localPointToGlobal(modeBtn.getBounds().getBottomLeft());
	menu.showMenuAsync(PopupMenu::Options()
		.withTargetComponent(*this)
		.withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
		[this](int result) {
			if (result == 0) return;

			auto param = editor.audioProcessor.params.getParameter(prefix + "mode");
			param->setValueNotifyingHost(param->convertTo0to1((float)(result - 1)));
		});
}
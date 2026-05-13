#include "Header.h"
#include "../../PluginEditor.h"


void CPUMeter::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();
	g.setColour(COLOR_PANEL());
	g.fillRoundedRectangle(b, 3.f);

    g.setFont(FontOptions(16.f));
    g.setColour(COLOR_KNOB_LABEL());

	auto pad = 0.f;
	auto w = (b.getWidth() - pad * 2);
	auto x = pad + w / 4 / 2;

	UIUtils::drawCPU(g, Rectangle<float>(x - 6, 6.f, 16.f, 16.f), COLOR_KNOB_LABEL());
	x += w / 4;

    g.drawText(String((int)editor.audioProcessor.synth->getCpuUsage()), Rectangle<float>(x - 16.f, 6.f, 32.f, 16.f), Justification::centred);
	x += w / 4;

	UIUtils::drawVoices(g, Rectangle<float>(x - 6, 6.f, 16.f, 16.f), COLOR_KNOB_LABEL());
	x += w / 4;

    g.drawText(String(editor.audioProcessor.synth->getNumActiveVoices()), Rectangle<float>(x - 16.f, 6.f, 32.f, 16.f), Justification::centred);
	
}

///////////////////////////////////////////////

Header::Header(TetraOPAudioProcessorEditor& e)
	: editor(e)
{
	startTimerHz(5);

	addAndMakeVisible(logo);
	logo.setAlpha(0.f);
	logo.onClick = [this] { editor.showAboutDialog();};

	addAndMakeVisible(lib);
	lib.setAlpha(0.f);
	lib.onClick = [this]
		{
			editor.selectTab(4);
			toggleUIComponents();
		};

	addAndMakeVisible(syn);
	syn.setAlpha(0.f);
	syn.onClick = [this]
		{
			editor.selectTab(0);
			toggleUIComponents();
		};

	addAndMakeVisible(fxs);
	fxs.setAlpha(0.f);
	fxs.onClick = [this]
		{
			editor.selectTab(1);
			toggleUIComponents();
		};

	addAndMakeVisible(mod);
	mod.setAlpha(0.f);
	mod.onClick = [this]
		{
			editor.selectTab(2);
			toggleUIComponents();
		};

	addAndMakeVisible(cfg);
	cfg.setAlpha(0.f);
	cfg.onClick = [this]
		{
			editor.selectTab(3);
			toggleUIComponents();
		};

	addAndMakeVisible(prevPreset);
	prevPreset.setAlpha(0.f);

	addAndMakeVisible(nextPreset);
	nextPreset.setAlpha(0.f);

	addAndMakeVisible(preset);
	preset.setAlpha(0.f);

	addAndMakeVisible(saveBtn);
	saveBtn.setAlpha(0.f);

	addAndMakeVisible(menuBtn);
	menuBtn.setAlpha(0.f);

	cpuMeter = std::make_unique<CPUMeter>(editor);
	addAndMakeVisible(cpuMeter.get());
}

Header::~Header()
{
}

void Header::timerCallback()
{
	if (editor.audioProcessor.presetmgr->nameDirty.exchange(false)) {
		repaint();
	}
}

void Header::parameterChanged(const juce::String&, float)
{
	MessageManager::callAsync([this]{ toggleUIComponents(); });
}

void Header::paint(Graphics& g)
{
	g.setColour(COLOR_PANEL().darker(0.6f));
    g.fillRect(getLocalBounds());

	UIUtils::drawLogo(g, logo.getBounds().reduced(5).toFloat(), COLOR_KNOB_LABEL());

	g.setFont(16.f);
	g.drawText("LIB", lib.getBounds().toFloat(), Justification::centred);
	g.drawText("SYN", syn.getBounds().toFloat(), Justification::centred);
	g.drawText("FXS", fxs.getBounds().toFloat(), Justification::centred);
	g.drawText("MOD", mod.getBounds().toFloat(), Justification::centred);
	g.drawText("CFG", cfg.getBounds().toFloat(), Justification::centred);

	int tab = editor.audioProcessor.selectedTab;
	auto selbounds = (tab == 0 ? syn.getBounds()
		: tab == 1 ? fxs.getBounds()
		: tab == 2 ? mod.getBounds()
		: tab == 3 ? cfg.getBounds()
		: lib.getBounds()).toFloat();

	g.setColour(COLOR_BACKGROUND());
	g.fillRect(selbounds);
	g.fillRoundedRectangle(prevPreset.getBounds().withRight(menuBtn.getRight()).toFloat(), 5.f);

	auto txt = tab == 0 ? "SYN"
		: tab == 1 ? "FXS"
		: tab == 2 ? "MOD"
		: tab == 3 ? "CFG"
		: "LIB";

	g.setColour(COLOR_ACTIVE());
	g.drawText(txt, selbounds, Justification::centred);

	g.setColour(COLOR_KNOB_LABEL());
	g.drawFittedText(editor.audioProcessor.presetmgr->selectedPreset.name, preset.getBounds(), Justification::centred, 1);

	UIUtils::drawTriangle(g, prevPreset.getBounds().toFloat().reduced(10.f), 3, COLOR_KNOB_LABEL());
	UIUtils::drawTriangle(g, nextPreset.getBounds().toFloat().reduced(10.f), 1, COLOR_KNOB_LABEL());
	UIUtils::drawSave(g, saveBtn.getBounds().reduced(5).toFloat(), COLOR_KNOB_LABEL());
	UIUtils::drawEllipsis(g, menuBtn.getBounds().toFloat().translated(-2,6), COLOR_KNOB_LABEL());
}

void Header::resized()
{
	logo.setBounds(0, 0, HEADER_HEIGHT, HEADER_HEIGHT);
	lib.setBounds(70, 0, 56, HEADER_HEIGHT);
	syn.setBounds(lib.getBounds().translated(lib.getWidth(), 0));
	fxs.setBounds(syn.getBounds().translated(syn.getWidth(), 0));
	mod.setBounds(fxs.getBounds().translated(fxs.getWidth(), 0));
	cfg.setBounds(mod.getBounds().translated(mod.getWidth(), 0));

	prevPreset.setBounds(365, 8, 28, 28);
	nextPreset.setBounds(prevPreset.getRight(), 8, 28, 28);
	preset.setBounds(nextPreset.getRight(), 8, 200, 28);
	saveBtn.setBounds(preset.getRight(), 8, 28, 28);
	menuBtn.setBounds(saveBtn.getRight(), 8, 28, 16);

	cpuMeter->setBounds(menuBtn.getRight() + 16, 8, 90, 28);
}


void Header::toggleUIComponents()
{
	MessageManager::callAsync([this] { repaint(); });
}
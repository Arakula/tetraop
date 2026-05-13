#include "Header.h"
#include "../../PluginEditor.h"


void CPUMeter::paint(Graphics& g)
{
	auto b = getLocalBounds();
    g.setFont(FontOptions(16.f));
    g.setColour(Colours::white);
    g.drawText(String("CPU ") + String((int)editor.audioProcessor.synth->getCpuUsage()), b, Justification::centredLeft);
    g.drawText(String("V ") + String(editor.audioProcessor.synth->getNumActiveVoices()), b, Justification::centredRight);
}

///////////////////////////////////////////////

Header::Header(TetraOPAudioProcessorEditor& e)
	: editor(e)
{
	addAndMakeVisible(logo);
	logo.setAlpha(0.f);

	addAndMakeVisible(lib);
	lib.setAlpha(0.f);

	addAndMakeVisible(syn);
	syn.setAlpha(0.f);

	addAndMakeVisible(fxs);
	fxs.setAlpha(0.f);

	addAndMakeVisible(mod);
	mod.setAlpha(0.f);

	addAndMakeVisible(cfg);
	cfg.setAlpha(0.f);

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

void Header::parameterChanged(const juce::String&, float)
{
	MessageManager::callAsync([this]{ toggleUIComponents(); });
}

void Header::paint(Graphics& g)
{
	g.setColour(COLOR_PANEL().darker(0.6f));
    g.fillRect(getLocalBounds());
}

void Header::resized()
{
	logo.setBounds(0, 0, HEADER_HEIGHT, HEADER_HEIGHT);
	lib.setBounds(70, 0, 56, HEADER_HEIGHT);
	syn.setBounds(lib.getBounds().translated(lib.getWidth(), 0));
	fxs.setBounds(syn.getBounds().translated(syn.getWidth(), 0));
	mod.setBounds(fxs.getBounds().translated(fxs.getWidth(), 0));
	cfg.setBounds(mod.getBounds().translated(mod.getWidth(), 0));

	prevPreset.setBounds(400, 8, 28, 28);
	nextPreset.setBounds(prevPreset.getRight(), 8, 28, 28);
	preset.setBounds(nextPreset.getRight(), 8, 200, 28);
	saveBtn.setBounds(preset.getRight(), 8, 28, 28);
	menuBtn.setBounds(saveBtn.getRight(), 8, 28, 28);

	cpuMeter->setBounds(700, 8, 90, 28);
}


void Header::toggleUIComponents()
{
	MessageManager::callAsync([this] { repaint(); });
}
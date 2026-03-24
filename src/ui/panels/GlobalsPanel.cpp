#include "GlobalsPanel.h"
#include "../../PluginEditor.h"

GlobalsPanel::GlobalsPanel(TetraOPAudioProcessorEditor& e)
	: editor(e)
{
	layout = std::make_unique<LayoutPicker>(editor);
	addAndMakeVisible(layout.get());
}

GlobalsPanel::~GlobalsPanel()
{
}

void GlobalsPanel::parameterChanged(const juce::String&, float)
{
	MessageManager::callAsync([this]{ toggleUIComponents(); });
}

void GlobalsPanel::paint(Graphics& g)
{
	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(FontOptions(16.f));
	g.drawText("Globals", 5, 0, 80, 25, Justification::centredLeft);
}

void GlobalsPanel::resized()
{
	auto b = getLocalBounds();

	layout->setBounds(0,25,80,80);
}


void GlobalsPanel::toggleUIComponents()
{
	MessageManager::callAsync([this] { repaint(); });
}
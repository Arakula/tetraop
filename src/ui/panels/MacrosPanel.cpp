#include "MacrosPanel.h"
#include "../../PluginEditor.h"

MacrosPanel::MacrosPanel(TetraOPAudioProcessorEditor& e)
	: editor(e)
{
}

MacrosPanel::~MacrosPanel()
{
}

void MacrosPanel::parameterChanged(const juce::String&, float)
{
	MessageManager::callAsync([this]{ toggleUIComponents(); });
}

void MacrosPanel::paint(Graphics& g)
{
}

void MacrosPanel::resized()
{
}


void MacrosPanel::toggleUIComponents()
{
	MessageManager::callAsync([this] { repaint(); });
}
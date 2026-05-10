#include "ModulatorsPanel.h"
#include "../../PluginEditor.h"

ModulatorsPanel::ModulatorsPanel(TetraOPAudioProcessorEditor& e)
	: editor(e)
{
}

ModulatorsPanel::~ModulatorsPanel()
{
}

void ModulatorsPanel::parameterChanged(const juce::String&, float)
{
	MessageManager::callAsync([this]{ toggleUIComponents(); });
}

void ModulatorsPanel::paint(Graphics& g)
{
	(void)g;
}

void ModulatorsPanel::resized()
{
}


void ModulatorsPanel::toggleUIComponents()
{
	MessageManager::callAsync([this] { repaint(); });
}
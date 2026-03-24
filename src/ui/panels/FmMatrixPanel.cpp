#include "FmMatrixPanel.h"
#include "../../PluginEditor.h"

FmMatrixPanel::FmMatrixPanel(TetraOPAudioProcessorEditor& e)
	: editor(e)
{
}

FmMatrixPanel::~FmMatrixPanel()
{
}

void FmMatrixPanel::parameterChanged(const juce::String&, float)
{
}

void FmMatrixPanel::paint(Graphics& g)
{
	(void)g;
}

void FmMatrixPanel::resized()
{
}


void FmMatrixPanel::toggleUIComponents()
{
	MessageManager::callAsync([this] { repaint(); });
}
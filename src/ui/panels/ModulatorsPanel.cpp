#include "ModulatorsPanel.h"
#include "../../PluginEditor.h"

ModulatorsPanel::ModulatorsPanel(TetraOPAudioProcessorEditor& e)
	: editor(e)
{
	vel = std::make_unique<Modulator>(editor, "vel");
	addAndMakeVisible(vel.get());

	key = std::make_unique<Modulator>(editor, "key");
	addAndMakeVisible(key.get());

	at = std::make_unique<Modulator>(editor, "at");
	addAndMakeVisible(at.get());

	rand = std::make_unique<Modulator>(editor, "rand");
	addAndMakeVisible(rand.get());

	mwheel = std::make_unique<Modulator>(editor, "mod");
	addAndMakeVisible(mwheel.get());
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
	auto b = getLocalBounds().toFloat();
	UIUtils::drawPanel(g, b, false, true);
}

void ModulatorsPanel::resized()
{
	auto bounds = getLocalBounds();
	auto gap = 2;

	vel->setBounds(gap, gap, bounds.getWidth() - gap * 2, 37);
	key->setBounds(vel->getBounds().translated(0, vel->getHeight() + gap));
	at->setBounds(key->getBounds().translated(0, key->getHeight() + gap));
	rand->setBounds(at->getBounds().translated(0, at->getHeight() + gap));
	mwheel->setBounds(rand->getBounds().translated(0, rand->getHeight() + gap));
}


void ModulatorsPanel::toggleUIComponents()
{
	MessageManager::callAsync([this] { repaint(); });
}
#include "MacrosPanel.h"
#include "../../PluginEditor.h"

MacrosPanel::MacrosPanel(TetraOPAudioProcessorEditor& e)
	: editor(e)
{
    for (int i = 0; i < globals::MAX_MACROS; ++i) {
        auto macro = std::make_unique<Macro>(editor, i);
        addAndMakeVisible(macro.get());
        macros.push_back(std::move(macro));
    }
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
    auto b = getLocalBounds().toFloat();
    UIUtils::drawPanel(g, b, false, true);
}

void MacrosPanel::resized()
{
    auto bounds = getLocalBounds().toFloat();

    float gap = 2.f;
    auto modw = bounds.getWidth() - gap * 2.f;
    auto modh = bounds.getHeight() / 4.f - 2.f;
    for (int i = 0; i < globals::MAX_MACROS; ++i) {
        auto& macro = macros[i];
        macro->setBounds(int(bounds.getX() + gap), int(bounds.getY() + i * gap + i * modh + gap), (int)modw, (int)modh);
    }
}


void MacrosPanel::toggleUIComponents()
{
	MessageManager::callAsync([this] { repaint(); });
}
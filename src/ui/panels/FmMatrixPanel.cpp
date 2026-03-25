#include "FmMatrixPanel.h"
#include "../../PluginEditor.h"

FmMatrixPanel::FmMatrixPanel(TetraOPAudioProcessorEditor& e)
	: editor(e)
{    
	String prefix[4] = { "a", "b", "c", "d" };

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			int idx = i * 4 + j;
			String fmparam = i == j ? prefix[i] + "_feedback" : "fm_" + prefix[j] + prefix[i];
			String rmparam = "rm_" + prefix[j] + prefix[i];
			fm[idx] = std::make_unique<Rotary>(editor, fmparam, "", Rotary::Percent);
			rm[idx] = std::make_unique<Rotary>(editor, rmparam, "", Rotary::Percent);
			addAndMakeVisible(fm[idx].get());
			addChildComponent(rm[idx].get());
			fm[idx]->setDarkSmall();
			rm[idx]->setDarkSmall();
			editor.registerModParam(fm[idx].get());
			editor.registerModParam(fm[idx].get());
			fm[idx]->colorValue = j == 0 ? COLOR_A() : j == 1 ? COLOR_B() : j == 2 ? COLOR_C() : COLOR_D();
			rm[idx]->colorValue = j == 0 ? COLOR_A() : j == 1 ? COLOR_B() : j == 2 ? COLOR_C() : COLOR_D();
		}
	}

	for (int i = 0; i < 4; ++i)
	{
		out[i] = std::make_unique<Rotary>(editor, "fm_" + prefix[i] + "out", "", Rotary::Percent);
		addAndMakeVisible(out[i].get());
		editor.registerModParam(out[i].get());
		out[i]->setDarkSmall();
		out[i]->colorValue = i == 0 ? COLOR_A() : i == 1 ? COLOR_B() : i == 2 ? COLOR_C() : COLOR_D();
	}
}

FmMatrixPanel::~FmMatrixPanel()
{
}

void FmMatrixPanel::parameterChanged(const juce::String&, float)
{
}

void FmMatrixPanel::paint(Graphics& g)
{
	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(FontOptions(16.f));
	g.drawText("Matrix", 5, 0, 80, 25, Justification::centredLeft);

	g.setFont(16.f);
	g.setColour(Colours::white.withAlpha(0.5f));
	g.drawText("A", fm[0]->getBounds().withWidth(25).translated(-25, -8), Justification::centred);
	g.drawText("B", fm[4]->getBounds().withWidth(25).translated(-25, -8), Justification::centred);
	g.drawText("C", fm[8]->getBounds().withWidth(25).translated(-25, -8), Justification::centred);
	g.drawText("D", fm[12]->getBounds().withWidth(25).translated(-25, -8), Justification::centred);
	g.setFont(FontOptions(12.f));
	g.drawText("OUT", out[0]->getBounds().withWidth(30).translated(-25, -8), Justification::centred);
}

void FmMatrixPanel::resized()
{
	auto b = getLocalBounds().withTrimmedTop(30).withTrimmedLeft(25);
	int w = 43;
	int h = 45;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			auto idx = i * 4 + j;
			fm[idx]->setBounds(b.getX() + j * w, b.getY() + i * h, w, h);
			rm[idx]->setBounds(b.getX() + j * w, b.getY() + i * h, w, h);
		}
	}

	for (int i = 0; i < 4; ++i)
	{
		out[i]->setBounds(b.getX() + i * w, b.getY() + 4 * h, w, h);
	}
}


void FmMatrixPanel::toggleUIComponents()
{
	MessageManager::callAsync([this] { repaint(); });
}
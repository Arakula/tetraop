#include "FmMatrixPanel.h"
#include "../../PluginEditor.h"

FmMatrixPanel::FmMatrixPanel(TetraOPAudioProcessorEditor& e)
	: editor(e)
{    
	addAndMakeVisible(closeBtn);
	closeBtn.setAlpha(0.f);
	closeBtn.onClick = [this] 
		{
			editor.toggleFmMatrix(); 
		};

	addAndMakeVisible(fmBtn);
	fmBtn.setAlpha(0.f);
	fmBtn.onClick = [this]
		{
			editor.audioProcessor.showRMMatrix = false;
			toggleUIComponents();
		};

	addAndMakeVisible(rmBtn);
	rmBtn.setAlpha(0.f);
	rmBtn.onClick = [this]
		{
			editor.audioProcessor.showRMMatrix = true;
			toggleUIComponents();
		};


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
			fm[idx]->setMatrixBtn();
			rm[idx]->setMatrixBtn();
			editor.registerModParam(fm[idx].get(), TetraOPAudioProcessorEditor::kFmMatrix);
			editor.registerModParam(fm[idx].get(), TetraOPAudioProcessorEditor::kFmMatrix);
			fm[idx]->colorValue = j == 0 ? COLOR_A() : j == 1 ? COLOR_B() : j == 2 ? COLOR_C() : COLOR_D();
			rm[idx]->colorValue = j == 0 ? COLOR_A() : j == 1 ? COLOR_B() : j == 2 ? COLOR_C() : COLOR_D();
		}
	}

	for (int i = 0; i < 4; ++i)
	{
		out[i] = std::make_unique<Rotary>(editor, "fm_" + prefix[i] + "out", "", Rotary::Percent);
		addAndMakeVisible(out[i].get());
		editor.registerModParam(out[i].get(), TetraOPAudioProcessorEditor::kFmMatrix);
		out[i]->setMatrixBtn();
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
	g.setColour(COLOR_PANEL().darker(0.6f));
	g.fillRect(getLocalBounds());

	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(FontOptions(16.f));
	g.drawText("Matrix", 5, 0, 80, 25, Justification::centredLeft);
	UIUtils::drawClose(g, closeBtn.getBounds().reduced(8).toFloat(), COLOR_KNOB_LABEL(), 2.f);

	bool showRM = editor.audioProcessor.showRMMatrix;
	g.fillRect((showRM ? rmBtn : fmBtn).getBounds().toFloat().reduced(0, 4));
	g.setColour(showRM ? COLOR_KNOB_LABEL() : COLOR_BACKGROUND());
	g.drawText("FM", fmBtn.getBounds(), Justification::centred);
	g.setColour(showRM ? COLOR_BACKGROUND() : COLOR_KNOB_LABEL());
	g.drawText("RM", rmBtn.getBounds(), Justification::centred);

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
	auto bounds = getLocalBounds();
	closeBtn.setBounds(Rectangle<int>(25, 25).withY(0).withRightX(bounds.getRight()));

	fmBtn.setBounds(Rectangle<int>(30, 25).withY(0).withRightX(bounds.getCentreX()));
	rmBtn.setBounds(Rectangle<int>(30, 25).withY(0).withX(bounds.getCentreX()));

	auto b = getLocalBounds().withTrimmedTop(30).withTrimmedLeft(25);
	int w = 43;
	int h = 50;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			auto idx = i * 4 + j;
			fm[idx]->setBounds(b.getX() + j * w, b.getY() + i * 43, w, h);
			rm[idx]->setBounds(b.getX() + j * w, b.getY() + i * 43, w, h);
		}
	}

	for (int i = 0; i < 4; ++i)
	{
		out[i]->setBounds(b.getX() + i * w, b.getY() + 4 * 43, w, h);
	}
}


void FmMatrixPanel::toggleUIComponents()
{
	for (auto& knob : fm)
		knob->setVisible(!editor.audioProcessor.showRMMatrix);

	for (auto& knob : rm)
		knob->setVisible(editor.audioProcessor.showRMMatrix);

	MessageManager::callAsync([this] { repaint(); });
}
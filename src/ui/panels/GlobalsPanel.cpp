#include "GlobalsPanel.h"
#include "../../PluginEditor.h"

GlobalsPanel::GlobalsPanel(TetraOPAudioProcessorEditor& e)
	: editor(e)
{
	editor.audioProcessor.params.addParameterListener("mono", this);
	editor.audioProcessor.params.addParameterListener("legato", this);

	layout = std::make_unique<LayoutPicker>(editor);
	addAndMakeVisible(layout.get());

	time = std::make_unique<Rotary>(editor, "global_time", "Time", Rotary::TimeFactor, true);
	addAndMakeVisible(time.get());
	editor.registerModParam(time.get(), TetraOPAudioProcessorEditor::kGlobal);

	pitch = std::make_unique<Rotary>(editor, "global_pitch", "Pitch", Rotary::PitchSemis, true);
	addAndMakeVisible(pitch.get());
	editor.registerModParam(pitch.get(), TetraOPAudioProcessorEditor::kGlobal);

	vel = std::make_unique<Rotary>(editor, "vel_sense", "Vel", Rotary::Percent);
	addAndMakeVisible(vel.get());

	glide = std::make_unique<Rotary>(editor, "glide", "Glide", Rotary::millis);
	addAndMakeVisible(glide.get());

	time->setDark();
	pitch->setDark();
	vel->setDark();
	glide->setDark();

	glideTension = std::make_unique<PowerCurve>(editor, "glide_tension", false);
	addAndMakeVisible(glideTension.get());

	poly = std::make_unique<ValuePicker>(editor, "polyphony");
	poly->isInteger = true;
	poly->precision = 0;
	addAndMakeVisible(poly.get());

	bend = std::make_unique<ValuePicker>(editor, "pitch_bend");
	bend->isInteger = true;
	bend->precision = 0;
	addAndMakeVisible(bend.get());

	addAndMakeVisible(monoBtn);
	monoBtn.setAlpha(0.f);
	monoBtn.onClick = [this]
		{
			auto param = editor.audioProcessor.params.getParameter("mono");
			param->setValueNotifyingHost(param->getValue() > 0.f ? 0.f : 1.f);
		};

	addAndMakeVisible(legatoBtn);
	legatoBtn.setAlpha(0.f);
	legatoBtn.onClick = [this]
		{
			auto param = editor.audioProcessor.params.getParameter("legato");
			param->setValueNotifyingHost(param->getValue() > 0.f ? 0.f : 1.f);
		};
}

GlobalsPanel::~GlobalsPanel()
{
	editor.audioProcessor.params.removeParameterListener("mono", this);
	editor.audioProcessor.params.removeParameterListener("legato", this);
}

void GlobalsPanel::parameterChanged(const juce::String&, float)
{
	MessageManager::callAsync([this]{ toggleUIComponents(); });
}

void GlobalsPanel::paint(Graphics& g)
{
	g.setColour(COLOR_PANEL().darker(0.6f));
	g.fillRect(getLocalBounds());

	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(FontOptions(16.f));
	g.drawText("Globals", 5, 0, 80, 25, Justification::centredLeft);

	g.setColour(COLOR_KNOB_LABEL().withAlpha(.5f));
	g.drawText("Poly", poly->getBounds().translated(0, -25), Justification::centred);
	g.drawText("Bend", bend->getBounds().translated(0, -25), Justification::centred);

	g.setColour(COLOR_KNOB_LABEL());
	g.drawText("Curve", glideTension->getBounds().withHeight(20).expanded(5, 0).translated(0, 34), Justification::centred);

	auto mono = (bool)editor.audioProcessor.params.getRawParameterValue("mono")->load();
	auto legato = (bool)editor.audioProcessor.params.getRawParameterValue("legato")->load();

	if (mono)
	{
		g.setColour(COLOR_ACTIVE());
		g.fillRoundedRectangle(monoBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f);
		g.setColour(COLOR_BACKGROUND());
		g.drawText("Mono", monoBtn.getBounds().toFloat(), Justification::centred);
	} 
	else
	{
		g.setColour(COLOR_ACTIVE());
		g.drawRoundedRectangle(monoBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f, 1.4f);
		g.drawText("Mono", monoBtn.getBounds().toFloat(), Justification::centred);
	}

	if (legato)
	{
		g.setColour(COLOR_ACTIVE());
		g.fillRoundedRectangle(legatoBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f);
		g.setColour(COLOR_BACKGROUND());
		g.drawText("Legato", legatoBtn.getBounds().toFloat(), Justification::centred);
	}
	else
	{
		g.setColour(COLOR_ACTIVE());
		g.drawRoundedRectangle(legatoBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f, 1.f);
		g.drawText("Legato", legatoBtn.getBounds().toFloat(), Justification::centred);
	}
}

void GlobalsPanel::resized()
{
	auto b = getLocalBounds().withTrimmedTop(25);
	layout->setBounds(b.getX() + 15, b.getY() + 15, 60, 60);

	time->setBounds(layout->getRight(), layout->getY() - 15, KNOB_WIDTH, KNOB_HEIGHT);
	pitch->setBounds(time->getBounds().translated(KNOB_WIDTH, 0));
	vel->setBounds(time->getX() - KNOB_WIDTH, time->getBottom(), KNOB_WIDTH, KNOB_HEIGHT);
	glide->setBounds(vel->getRight(), vel->getY(), KNOB_WIDTH, KNOB_HEIGHT);
	glideTension->setBounds(glide->getBounds().translated(KNOB_WIDTH, 0).reduced(10, 20));

	poly->setBounds(vel->getBounds().withHeight(20).reduced(5, 0).translated(0, int(KNOB_HEIGHT * 1.5f)));
	bend->setBounds(glide->getBounds().withHeight(20).reduced(5, 0).translated(0, int(KNOB_HEIGHT * 1.5f)));

	monoBtn.setBounds(bend->getBounds().translated(KNOB_WIDTH, -25).withWidth(KNOB_WIDTH));
	legatoBtn.setBounds(monoBtn.getBounds().translated(0, 25));
}


void GlobalsPanel::toggleUIComponents()
{
	MessageManager::callAsync([this] { repaint(); });
}
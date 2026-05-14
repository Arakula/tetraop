#include "ConfigsPanel.h"
#include "../../PluginEditor.h"

ConfigsPanel::ConfigsPanel(TetraOPAudioProcessorEditor& e)
	: editor(e)
{
	startTimerHz(10);
	editor.audioProcessor.params.addParameterListener("set_mpe_enabled", this);
	editor.audioProcessor.params.addParameterListener("set_retrigger_noise", this);
	editor.audioProcessor.params.addParameterListener("set_retrigger_mallet_noise", this);

	scaleBtn.getProperties().set ("layoutID", "scaleBtn");
	addAndMakeVisible(scaleBtn);
	currscale = editor.audioProcessor.scale;
	scaleBtn.setButtonText(juce::String(juce::roundToInt(currscale * 100.f)) + "%");
	scaleBtn.setColour(TextButton::ColourIds::buttonColourId, Colours::transparentBlack);

	scaleBtn.onClick = [this]() { showScaleMenu(); };

	mpeEnabledBtn.setName ("mpeEnabledBtn");
	addAndMakeVisible(mpeEnabledBtn);
	mpeEnabledBtn.setAlpha(0.f);
	mpeEnabledBtn.onClick = [this]
		{
			auto param = editor.audioProcessor.params.getParameter("mpe");
			param->setValueNotifyingHost(param->getValue() < 1.0f ? 1.f : 0.f);
		};

	velEditor = std::make_unique<CurveEditor>(editor,
		&editor.audioProcessor.modulation->velCurve, 5.f,
		COLOR_ACTIVE(), COLOR_LFO(), false, false
	);
	velEditor->drawShade = false;
	velEditor->drawBasicSeek = true;
	velEditor->useGrid = false;
	velEditor->onChange = [this]()
		{
			editor.audioProcessor.saveSettings();
		};
	velEditor->setName ("velEditor");
	addAndMakeVisible(velEditor.get());

	unboundedMouseBtn.setName ("unboundedMouseBtn");
	addAndMakeVisible(unboundedMouseBtn);
	unboundedMouseBtn.setAlpha(0.f);
	unboundedMouseBtn.onClick = [this]
		{
			editor.audioProcessor.unboundedMouse = !editor.audioProcessor.unboundedMouse;
			editor.audioProcessor.saveSettings();
			repaint();
		};
}

ConfigsPanel::~ConfigsPanel()
{
	editor.audioProcessor.params.removeParameterListener("set_mpe_enabled", this);
	editor.audioProcessor.params.removeParameterListener("set_retrigger_noise", this);
	editor.audioProcessor.params.removeParameterListener("set_retrigger_mallet_noise", this);
}

void ConfigsPanel::timerCallback()
{
	if (!isVisible())
		return;

	velEditor->seekPos = editor.audioProcessor.modulation->lastVel;
	velEditor->repaint();

	if (currscale != editor.audioProcessor.scale) {
		currscale = editor.audioProcessor.scale;
		scaleBtn.setButtonText(juce::String(juce::roundToInt(currscale * 100.f)) + "%");
	}
}

void ConfigsPanel::parameterChanged(const juce::String& parameterID, float newValue)
{
	(void)parameterID;
	(void)newValue;
	juce::MessageManager::callAsync([this]() { toggleUIComponents(); });
}

void ConfigsPanel::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds().toFloat();
	UIUtils::drawPanel(g, bounds, false, false);

	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(juce::FontOptions(16.f));
	g.drawText("UI Scale", scaleBtn.getBounds().withX(scaleBtn.getRight() + 15), Justification::centredLeft);
	g.drawText("Velocity Map", velEditor->getBounds().translated(0, -25).withHeight(25), Justification::centredLeft);

	//auto mpeEnabled = (bool)editor.audioProcessor.params.getRawParameterValue("mpe")->load();

	//UIUtils::drawCheckmark(g, juce::Rectangle<float>(13.f, 13.f)
	//	.withX((float)mpeEnabledBtn.getX())
	//	.withY((float)mpeEnabledBtn.getBounds().getCentreY() - 13 / 2.f)
	//	, COLOR_BEVEL(), COLOR_ACTIVE(), mpeEnabled);
	//g.setColour(COLOR_KNOB_LABEL());
	//g.setFont(juce::FontOptions(16.f));
	//g.drawText("MPE Enabled", mpeEnabledBtn.getBounds().withTrimmedLeft(22), juce::Justification::centredLeft);

	UIUtils::drawCheckmark(g, juce::Rectangle<float>(13.f, 13.f)
		.withX((float)unboundedMouseBtn.getX())
		.withY((float)unboundedMouseBtn.getBounds().getCentreY() - 13 / 2.f)
		, COLOR_BEVEL(), COLOR_ACTIVE(), editor.audioProcessor.unboundedMouse);
	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(juce::FontOptions(16.f));
	g.drawText("Unbounded Mouse", unboundedMouseBtn.getBounds().withTrimmedLeft(22), juce::Justification::centredLeft);

	UIUtils::drawBevel(g, velEditor->getBounds().toFloat(), 4.f, COLOR_BACKGROUND());

	auto b = Rectangle<float>(500.f, 20.f, 200.f, 80.f).withRightX(bounds.getRight() - 20.f);
	g.setColour(COLOR_KNOB_LABEL());
	g.drawText("TetraOP", b, juce::Justification::centredTop);
	g.drawText(juce::String(ProjectInfo::versionString), b.withTrimmedTop(30.f), juce::Justification::centredTop);
	g.setFont(juce::FontOptions(12.f));
	g.setColour(COLOR_KNOB_LABEL());
	g.drawText(juce::String(__DATE__), b.withTrimmedTop(53.f), juce::Justification::centredTop);
}

void ConfigsPanel::resized()
{
	auto bounds = getLocalBounds().toFloat();
	int pad = 20;

	int x = pad;
	int y = pad;

	scaleBtn.setBounds(x, y, 60, 25);
	y += 25;

	mpeEnabledBtn.setBounds(x, y, 200, 25);
	y += 25;

	unboundedMouseBtn.setBounds(x, y, 200, 25);

	// column 2
	y = pad;
	x = pad + 200;

	velEditor->setBounds(x, y + 25, 200, 100);
	y += 20 + 50;


	toggleUIComponents();
}

void ConfigsPanel::toggleUIComponents()
{
	repaint();
}

void ConfigsPanel::showScaleMenu()
{
	auto scl = editor.audioProcessor.scale;

	juce::PopupMenu menu;
	menu.addItem(1, "100%", true, scl == 1.f);
	menu.addItem(2, "110%", true, scl == 1.1f);
	menu.addItem(3, "125%", true, scl == 1.25f);
	menu.addItem(4, "150%", true, scl == 1.5f);
	menu.addItem(5, "175%", true, scl == 1.75f);
	menu.addItem(6, "200%", true, scl == 2.f);

	auto menuPos = localPointToGlobal(scaleBtn.getBounds().getBottomLeft());
	menu.setLookAndFeel(&getLookAndFeel());
	menu.showMenuAsync(juce::PopupMenu::Options()
		.withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
		.withTargetComponent(*this)
		.withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
		[this](int result) {
			if (result == 0) return;
			auto _scl = result == 1 ? 1.f
				: result == 2 ? 1.1f
				: result == 3 ? 1.25f
				: result == 4 ? 1.5f
				: result == 5 ? 1.75f
				: 2.f;

			editor.audioProcessor.scale = _scl;
			editor.audioProcessor.saveSettings();

            ScaledPluginEditor::updateSize (*this);
		});
}
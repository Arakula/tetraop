#include "FXCompressor.h"
#include "../../PluginEditor.h"

FXCompressor::FXCompressor(TetraOPAudioProcessorEditor& e)
	: UIFX(e, FX::Compressor)
{
	editor.audioProcessor.params.addParameterListener(prefix + "makeup", this);
	startTimerHz(30);

	makeupBtn.setName ("makeupBtn");
	addAndMakeVisible(makeupBtn);
	makeupBtn.setAlpha(0.f);

	makeupBtn.onClick = [this]
		{
			auto param = editor.audioProcessor.params.getParameter(prefix + "makeup");
			param->setValueNotifyingHost(param->getValue() > 0.f ? 0.f : 1.f);
		};

	thresh = std::make_unique<Rotary>(editor, prefix + "thresh", "Thresh", Rotary::dB);
	ratio = std::make_unique<Rotary>(editor, prefix + "ratio", "Ratio", Rotary::float1);
	attack = std::make_unique<Rotary>(editor, prefix + "att", "Attack", Rotary::millis);
	release = std::make_unique<Rotary>(editor, prefix + "rel", "Release", Rotary::millis);
	gain = std::make_unique<Rotary>(editor, prefix + "gain", "Gain", Rotary::dB, true);

	addAndMakeVisible(thresh.get());
	addAndMakeVisible(ratio.get());
	addAndMakeVisible(attack.get());
	addAndMakeVisible(release.get());
	addAndMakeVisible(gain.get());

	editor.registerModParam(thresh.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(ratio.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(attack.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(release.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(gain.get(), TetraOPAudioProcessorEditor::kFX);
}

FXCompressor::~FXCompressor()
{
	editor.audioProcessor.params.removeParameterListener(prefix + "makeup", this);
}

void FXCompressor::timerCallback()
{
	if (isShowing()) {
		repaint();
	}
}

void FXCompressor::onActiveToggle()
{
	thresh->setEnabled(on);
	ratio->setEnabled(on);
	attack->setEnabled(on);
	release->setEnabled(on);
	gain->setEnabled(on);
}

void FXCompressor::parameterChanged(const juce::String& parameterID, float newValue)
{
	(void)parameterID;
	(void)newValue;
	juce::MessageManager::callAsync([this] { toggleUIComponents(); });
}

void FXCompressor::mouseDown(const juce::MouseEvent& e)
{
	UIFX::mouseDown(e);
}

void FXCompressor::paint(juce::Graphics& g)
{
	UIFX::paint(g);
	bool makeup = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "makeup")->load();
	UIUtils::drawBevelLight(g, makeupBtn.getBounds().toFloat(), makeup);
	g.setFont(juce::FontOptions(16.f));
	g.setColour(makeup ? COLOR_BEVEL() : COLOR_KNOB_LABEL());
	g.drawText("Makeup", makeupBtn.getBounds().toFloat(), juce::Justification::centred);

	g.setColour(COLOR_KNOB_LABEL());
	g.drawText("Reduction", redBounds.translated(0, -20), Justification::centred);

	auto redgain = editor.audioProcessor.compReduction.load();

	g.setColour(COLOR_ACTIVE().darker(0.5f));
	g.fillRect(redBounds);
	g.setColour(COLOR_ACTIVE());
	g.fillRect(redBounds.withWidth((int)(redBounds.getWidth() * std::min(1.f, 1.f - redgain))));
}

void FXCompressor::resized()
{
	UIFX::resized();

	release->setBounds(Rectangle<int>(KNOB_WIDTH, KNOB_HEIGHT).withX(KNOB_WIDTH)
		.withBottomY(getBottom() - 10 - PANEL_PAD - KNOB_HEIGHT));
	
	attack->setBounds(release->getBounds().translated(-KNOB_WIDTH, 0));

	thresh->setBounds(attack->getBounds().translated(0, -KNOB_HEIGHT));
	ratio->setBounds(thresh->getBounds().translated(KNOB_WIDTH, 0));
	
	gain->setBounds(Rectangle<int>(KNOB_WIDTH, KNOB_HEIGHT).withX(KNOB_WIDTH)
		.withBottomY(getBottom() - 10 - PANEL_PAD));

	makeupBtn.setBounds(gain->getBounds().translated(-KNOB_WIDTH, 0)
		.withHeight(25).translated(0, 25));

	auto b = getLocalBounds();
	redBounds = Rectangle<int>(70, 10)
		.withX(b.getCentreX() - 70 / 2)
		.withY(PANEL_HEADER_HEIGHT + 70);

	toggleUIComponents();
}

void FXCompressor::toggleUIComponents()
{
	repaint();
}

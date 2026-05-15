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

	thresh->setName  ("thresh");
	ratio->setName   ("ratio");
	attack->setName  ("attack");
	release->setName ("release");
	gain->setName    ("gain");

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

void FXCompressor::parameterChanged(const juce::String& parameterID, float newValue)
{
	(void)parameterID;
	(void)newValue;
	juce::MessageManager::callAsync([this] { toggleUIComponents(); });
}

void FXCompressor::mouseDown(const juce::MouseEvent& e)
{
	(void)e;
}

void FXCompressor::paint(juce::Graphics& g)
{
	UIFX::paint(g);
	bool makeup = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "makeup")->load();
	UIUtils::drawBevel(g, makeupBtn.getBounds().toFloat().translated(0.5f, 0.5f), 3.f, makeup
		? COLOR_ACTIVE() : COLOR_BEVEL());
	g.setFont(juce::FontOptions(15.f));
	g.setColour(makeup ? COLOR_BEVEL() : COLOR_KNOB_LABEL().withAlpha(0.5f));
	g.drawText("Makeup", makeupBtn.getBounds().toFloat(), juce::Justification::centred);

	auto redBounds = gain->getBounds().withWidth(6).withX(gain->getRight() + 2).reduced(0, 10);
	auto redgain = editor.audioProcessor.compReduction.load();

	g.setColour(COLOR_ACTIVE().darker(0.5f));
	g.fillRect(redBounds);
	g.setColour(COLOR_ACTIVE());
	g.fillRect(redBounds.withHeight((int)(redBounds.getHeight() * std::min(1.f, 1.f - redgain))));
}

void FXCompressor::resized()
{
	toggleUIComponents();
}

void FXCompressor::toggleUIComponents()
{
	repaint();
}

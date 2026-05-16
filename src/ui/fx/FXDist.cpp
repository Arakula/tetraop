#include "FXDist.h"
#include "../../PluginEditor.h"

FXDist::FXDist(TetraOPAudioProcessorEditor& e)
	: UIFX(e, FX::Distortion)
{
	editor.audioProcessor.params.addParameterListener(prefix + "mode", this);

	modeBtn.setName ("modeBtn");
	addAndMakeVisible(modeBtn);
	modeBtn.setAlpha(0.f);

	modeBtn.onClick = [this]
		{
			showModeMenu();
		};

	filter = std::make_unique<Rotary>(editor, prefix + "filter", "Filter", Rotary::FilterLPHP, true);
	drive = std::make_unique<Rotary>(editor, prefix + "drive", "Drive", Rotary::dB, false);
	color = std::make_unique<Rotary>(editor, prefix + "color", "Color", Rotary::FilterLPHP, true);
	gain = std::make_unique<Rotary>(editor, prefix + "gain", "Gain", Rotary::dB, true);
	mix = std::make_unique<Rotary>(editor, prefix + "mix", "Mix", Rotary::Percent);

	addAndMakeVisible(filter.get());
	addAndMakeVisible(drive.get());
	addAndMakeVisible(color.get());
	addAndMakeVisible(gain.get());
	addAndMakeVisible(mix.get());

	editor.registerModParam(filter.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(drive.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(color.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(gain.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(mix.get(), TetraOPAudioProcessorEditor::kFX);
}

FXDist::~FXDist()
{
	editor.audioProcessor.params.removeParameterListener(prefix + "mode", this);
}

void FXDist::parameterChanged(const juce::String& parameterID, float newValue)
{
	(void)parameterID;
	(void)newValue;
	juce::MessageManager::callAsync([this] { toggleUIComponents(); });
}

void FXDist::onActiveToggle()
{
	filter->setEnabled(on);
	drive->setEnabled(on);
	color->setEnabled(on);
	gain->setEnabled(on);
	mix->setEnabled(on);
}

void FXDist::mouseDown(const juce::MouseEvent& e)
{
	UIFX::mouseDown(e);
}

void FXDist::paint(juce::Graphics& g)
{
	UIFX::paint(g);

	auto mode = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "mode")->load();

	UIUtils::drawBevelLight(g, modeBtn.getBounds().toFloat(), false);
	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(juce::FontOptions(16.f));
	g.drawText(mode == 0 ? "Tube" : mode == 1 ? "SClip" : "HClip", modeBtn.getBounds(), juce::Justification::centred);
}

void FXDist::resized()
{
	UIFX::resized();

	mix->setBounds(Rectangle<int>(KNOB_WIDTH, KNOB_HEIGHT).withX(KNOB_WIDTH).withBottomY(getBottom() - 10 - PANEL_PAD));
	color->setBounds(mix->getBounds().translated(0, -KNOB_HEIGHT));
	filter->setBounds(color->getBounds().translated(-KNOB_WIDTH, 0));
	drive->setBounds(filter->getBounds().translated(0, -KNOB_HEIGHT));
	gain->setBounds(drive->getBounds().translated(KNOB_WIDTH, 0));

	auto b = getLocalBounds();

	modeBtn.setBounds(drive->getBounds().translated(0, -KNOB_HEIGHT)
		.withHeight(25)
		.withX(b.getCentreX() - KNOB_WIDTH / 2)
		.translated(0, 23));

	toggleUIComponents();
}

void FXDist::toggleUIComponents()
{
	repaint();
}

void FXDist::showModeMenu()
{
	auto mode = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "mode")->load();

	juce::PopupMenu menu;
	menu.addItem(1, "Tube", true, mode == 0);
	menu.addItem(2, "SoftClip", true, mode == 1);
	menu.addItem(3, "HardClip", true, mode == 2);

	auto menuPos = localPointToGlobal(modeBtn.getBounds().getBottomLeft());
	menu.setLookAndFeel(&getLookAndFeel());
	menu.showMenuAsync(juce::PopupMenu::Options()
		.withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
		.withTargetComponent(*this)
		.withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
		[this](int result) {
			if (result == 0) return;
			auto param = editor.audioProcessor.params.getParameter(prefix + "mode");
			param->setValueNotifyingHost(param->convertTo0to1((float)result - 1));
			toggleUIComponents();
		});
}

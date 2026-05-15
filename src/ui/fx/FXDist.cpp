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
	drive = std::make_unique<Rotary>(editor, prefix + "drive", "Drive", Rotary::dB, true);
	color = std::make_unique<Rotary>(editor, prefix + "color", "Color", Rotary::FilterLPHP, true);
	gain = std::make_unique<Rotary>(editor, prefix + "gain", "Gain", Rotary::dB, true);
	mix = std::make_unique<Rotary>(editor, prefix + "mix", "Mix", Rotary::Percent, true);

	filter->setName ("filter");
	drive->setName  ("drive");
	color->setName  ("color");
	gain->setName   ("gain");
	mix->setName    ("mix");

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

void FXDist::mouseDown(const juce::MouseEvent& e)
{
	UIFX::mouseDown(e);
}

void FXDist::paint(juce::Graphics& g)
{
	UIFX::paint(g);

	auto mode = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "mode")->load();

	UIUtils::drawBevel(g, modeBtn.getBounds().toFloat().expanded(0.5f), 3.f, COLOR_BEVEL());
	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(juce::FontOptions(15.f));
	g.drawText(mode == 0 ? "Tube" : mode == 1 ? "Tape" : "Fuzz", modeBtn.getBounds(), juce::Justification::centred);
}

void FXDist::resized()
{
	UIFX::resized();
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
	menu.addItem(2, "Tape", true, mode == 1);
	menu.addItem(3, "Fuzz", true, mode == 2);

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

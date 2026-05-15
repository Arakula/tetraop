#include "FXChorus.h"
#include "../../PluginEditor.h"

FXChorus::FXChorus(TetraOPAudioProcessorEditor& e)
	: UIFX(e, FX::Chorus)
{
	rate = std::make_unique<Rotary>(editor, prefix + "rate", "Rate", Rotary::Hz1f);
	depth = std::make_unique<Rotary>(editor, prefix + "depth", "Depth", Rotary::Percent);
	lowcut = std::make_unique<Rotary>(editor, prefix + "lowcut", "Lowcut", Rotary::Hz);
	highcut = std::make_unique<Rotary>(editor, prefix + "highcut", "Highcut", Rotary::Hz);
	highcut->invertValue = true;
	feedback = std::make_unique<Rotary>(editor, prefix + "feedback", "Feedbk", Rotary::Percent, true);
	mix = std::make_unique<Rotary>(editor, prefix + "mix", "Mix", Rotary::Percent, true);

	rate->setName     ("rate");
	depth->setName    ("depth");
	lowcut->setName   ("lowcut");
	highcut->setName  ("highcut");
	feedback->setName ("feedback");
	mix->setName      ("mix");

	addAndMakeVisible(rate.get());
	addAndMakeVisible(depth.get());
	addAndMakeVisible(lowcut.get());
	addAndMakeVisible(highcut.get());
	addAndMakeVisible(feedback.get());
	addAndMakeVisible(mix.get());

	editor.registerModParam(rate.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(depth.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(lowcut.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(highcut.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(feedback.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(mix.get(), TetraOPAudioProcessorEditor::kFX);
}

FXChorus::~FXChorus()
{
}

void FXChorus::mouseDown(const juce::MouseEvent& e)
{
	UIFX::mouseDown(e);
	if (voiceBounds.contains((float)e.x, (float)e.y)) {
		showVoicesMenu();
	}
}

void FXChorus::paint(juce::Graphics& g)
{
	UIFX::paint(g);

	auto bounds = getLocalBounds().toFloat();

	voiceBounds = juce::Rectangle<float>(50.f, 25.f).withX(95.f).withY(bounds.getCentreY() - 25 / 2.f);
	UIUtils::drawBevel(g, voiceBounds.translated(0.5f, 0.5f), 3.f, COLOR_BEVEL());
	g.setFont(juce::FontOptions(15.f));
	g.setColour(COLOR_KNOB_LABEL().withAlpha(0.5f));

	auto voices = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "voices")->load();
	g.drawText((voices == 0 ? "2" : juce::String(voices * 4)), voiceBounds, juce::Justification::centred);
}

void FXChorus::resized()
{
	UIFX::resized();
}

void FXChorus::showVoicesMenu()
{
	auto voices = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "voices")->load();
	voices = voices == 0 ? 2 : voices * 4;

	juce::PopupMenu menu;
	menu.addSectionHeader("Voices");
	menu.addItem(1, "2", true, voices == 2);
	menu.addItem(2, "4", true, voices == 4);
	menu.addItem(3, "8", true, voices == 8);
	menu.addItem(4, "12", true, voices == 12);
	menu.addItem(5, "16", true, voices == 16);

	auto menuPos = localPointToGlobal(voiceBounds.toNearestInt().getBottomLeft());
	menu.setLookAndFeel(&getLookAndFeel());
	menu.showMenuAsync(juce::PopupMenu::Options()
		.withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
		.withTargetComponent(*this)
		.withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
		[this](int result) {
			if (result == 0) return;
			auto param = editor.audioProcessor.params.getParameter(prefix + "voices");
			param->setValueNotifyingHost(param->convertTo0to1((float)(result - 1)));
			repaint();
		});
}

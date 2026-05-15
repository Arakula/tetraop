#include "FXDelay.h"
#include "../../PluginEditor.h"

FXDelay::FXDelay(TetraOPAudioProcessorEditor& e)
	: UIFX(e, FX::Delay)
{
	editor.audioProcessor.params.addParameterListener(prefix + "mode", this);
	editor.audioProcessor.params.addParameterListener(prefix + "link", this);
	editor.audioProcessor.params.addParameterListener(prefix + "sync_l", this);
	editor.audioProcessor.params.addParameterListener(prefix + "sync_r", this);

	syncModeLBtn.setName ("syncModeLBtn");
	syncModeRBtn.setName ("syncModeRBtn");
	linkBtn.setName      ("linkBtn");
	modeBtn.setName      ("modeBtn");

	addAndMakeVisible(syncModeLBtn);
	syncModeLBtn.setAlpha(0.f);
	addAndMakeVisible(syncModeRBtn);
	syncModeRBtn.setAlpha(0.f);
	addAndMakeVisible(linkBtn);
	linkBtn.setAlpha(0.f);
	addAndMakeVisible(modeBtn);
	modeBtn.setAlpha(0.f);

	syncModeLBtn.onClick = [this]
		{
			showSyncMenu(true);
		};

	syncModeRBtn.onClick = [this]
		{
			showSyncMenu(false);
		};

	modeBtn.onClick = [this]
		{
			showModeMenu();
		};

	linkBtn.onClick = [this]
		{
			auto param = editor.audioProcessor.params.getParameter(prefix + "link");
			param->setValueNotifyingHost(param->getValue() > 0.f ? 0.f : 1.f);
		};

	rateL = std::make_unique<TimePicker>(editor, prefix + "rate_l");
	rateSyncL = std::make_unique<TimePicker>(editor, prefix + "rate_sync_l");
	rateR = std::make_unique<TimePicker>(editor, prefix + "rate_r");
	rateSyncR = std::make_unique<TimePicker>(editor, prefix + "rate_sync_r");

	feedback = std::make_unique<Rotary>(editor, prefix + "feedback", "Feedbk", Rotary::Percent);
	lowcut = std::make_unique<Rotary>(editor, prefix + "lowcut", "Lowcut", Rotary::Hz);
	highcut = std::make_unique<Rotary>(editor, prefix + "highcut", "Highcut", Rotary::Hz);
	highcut->invertValue = true;
	width = std::make_unique<HSlider>(editor, prefix + "pipo_width", "Width", HSlider::Percent, true, COLOR_ACTIVE());
	width->showLabelPrefix = true;
	mix = std::make_unique<Rotary>(editor, prefix + "mix", "Mix", Rotary::Percent, true);

	width->drawBorder = false;

	rateL->setName     ("rateL");
	rateR->setName     ("rateR");
	rateSyncL->setName ("rateSyncL");
	rateSyncR->setName ("rateSyncR");
	feedback->setName  ("feedback");
	lowcut->setName    ("lowcut");
	highcut->setName   ("highcut");
	width->setName     ("width");
	mix->setName       ("mix");

	addAndMakeVisible(rateL.get());
	addAndMakeVisible(rateR.get());
	addAndMakeVisible(rateSyncL.get());
	addAndMakeVisible(rateSyncR.get());
	addAndMakeVisible(feedback.get());
	addAndMakeVisible(lowcut.get());
	addAndMakeVisible(highcut.get());
	addAndMakeVisible(width.get());
	addAndMakeVisible(mix.get());

	editor.registerModParam(rateL.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(rateSyncL.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(rateR.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(rateSyncR.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(feedback.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(lowcut.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(highcut.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(width.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(mix.get(), TetraOPAudioProcessorEditor::kFX);
}

FXDelay::~FXDelay()
{
	editor.audioProcessor.params.removeParameterListener(prefix + "mode", this);
	editor.audioProcessor.params.removeParameterListener(prefix + "link", this);
	editor.audioProcessor.params.removeParameterListener(prefix + "sync_l", this);
	editor.audioProcessor.params.removeParameterListener(prefix + "sync_r", this);
}

void FXDelay::parameterChanged(const juce::String& parameterID, float newValue)
{
	(void)parameterID;
	(void)newValue;
	juce::MessageManager::callAsync([this] { toggleUIComponents(); });
}

void FXDelay::mouseDown(const juce::MouseEvent& e)
{
	UIFX::mouseDown(e);
}

void FXDelay::paint(juce::Graphics& g)
{
	UIFX::paint(g);

	auto mode = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "mode")->load();
	auto modeL = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "sync_l")->load();
	auto modeR = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "sync_r")->load();
	auto link = (bool)editor.audioProcessor.params.getRawParameterValue(prefix + "link")->load();

	if (modeL == 0)
		UIUtils::drawClock(g, syncModeLBtn.getBounds().reduced(6).toFloat(), COLOR_KNOB_LABEL());
	else
		UIUtils::drawNote(g, syncModeLBtn.getBounds().reduced(6).toFloat(), modeL - 1, COLOR_KNOB_LABEL());
	if (modeR == 0)
		UIUtils::drawClock(g, syncModeRBtn.getBounds().reduced(6).toFloat(), COLOR_KNOB_LABEL());
	else
		UIUtils::drawNote(g, syncModeRBtn.getBounds().reduced(6).toFloat(), modeR - 1, COLOR_KNOB_LABEL());

	g.setColour(COLOR_KNOB_LABEL());
	g.drawText("L", rateL->getBounds().toFloat().translated(-10.f, 0.f), juce::Justification::centredLeft);
	g.drawText("R", rateR->getBounds().toFloat().translated(-10.f, 0.f), juce::Justification::centredLeft);

	UIUtils::drawChain(g, linkBtn.getBounds().toFloat().translated(-1.f, 16.f), link ? COLOR_DELAY() : COLOR_KNOB_LABEL());
	UIUtils::drawBevel(g, modeBtn.getBounds().toFloat().expanded(0.5f), 3.f, COLOR_BEVEL());

	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(juce::FontOptions(15.f));
	g.drawText(mode == 0 ? "Normal" : mode == 1 ? "PiPo" : "Tap", modeBtn.getBounds(), juce::Justification::centred);
}

void FXDelay::resized()
{
	UIFX::resized();
	toggleUIComponents();
}

void FXDelay::toggleUIComponents()
{
	auto mode = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "mode")->load();
	auto modeL = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "sync_l")->load();
	auto modeR = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "sync_r")->load();

	rateL->setVisible(modeL == 0);
	rateSyncL->setVisible(modeL > 0);
	rateR->setVisible(modeR == 0);
	rateSyncR->setVisible(modeR > 0);

	rateSyncL->mode = modeL;
	rateSyncR->mode = modeR;
	rateSyncL->repaint();
	rateSyncR->repaint();

	width->setVisible(mode == 1);

	repaint();
}

void FXDelay::showSyncMenu(bool isLeft)
{
	auto mode = (int)editor.audioProcessor.params.getRawParameterValue(isLeft ? prefix + "sync_l" : prefix + "sync_r")->load();

	juce::PopupMenu menu;
	menu.addItem(1, "Seconds", true, mode == 0);
	menu.addItem(2, "Straight", true, mode == 1);
	menu.addItem(3, "Triplet", true, mode == 2);
	menu.addItem(4, "Dotted", true, mode == 3);

	auto menuPos = localPointToGlobal((isLeft ? syncModeLBtn : syncModeRBtn).getBounds().getBottomLeft());
	menu.setLookAndFeel(&getLookAndFeel());
	menu.showMenuAsync(juce::PopupMenu::Options()
		.withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
		.withTargetComponent(*this)
		.withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
		[this, isLeft](int result) {
			if (result == 0) return;
			auto param = editor.audioProcessor.params.getParameter(isLeft ? prefix + "sync_l" : prefix + "sync_r");
			param->setValueNotifyingHost(param->convertTo0to1((float)(result - 1)));
			toggleUIComponents();
		});
}

void FXDelay::showModeMenu()
{
	auto mode = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "mode")->load();

	juce::PopupMenu menu;
	menu.addItem(1, "Normal", true, mode == 0);
	menu.addItem(2, "PingPong", true, mode == 1);
	menu.addItem(3, "Tap", true, mode == 2);

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

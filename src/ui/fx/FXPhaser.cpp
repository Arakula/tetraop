#include "FXPhaser.h"
#include "../../PluginEditor.h"

FXPhaser::FXPhaser(TetraOPAudioProcessorEditor& e)
	: UIFX(e, FX::Phaser)
{
    editor.audioProcessor.params.addParameterListener(prefix + "sync", this);

	center = std::make_unique<Rotary>(editor, prefix + "center", "Center", Rotary::Hz);
	depth = std::make_unique<Rotary>(editor, prefix + "depth", "Depth", Rotary::Percent);
	rate = std::make_unique<Rotary>(editor, prefix + "rate", "Rate", Rotary::Hz1f);
	rateSync = std::make_unique<Rotary>(editor, prefix + "rate_sync", "Rate", Rotary::Choice);
	res = std::make_unique<Rotary>(editor, prefix + "res", "Res", Rotary::Percent);
	morph = std::make_unique<Rotary>(editor, prefix + "morph", "Morph", Rotary::Percent);
	stereo = std::make_unique<Rotary>(editor, prefix + "stereo", "Stereo", Rotary::Percent);
	mix = std::make_unique<Rotary>(editor, prefix + "mix", "Mix", Rotary::Percent);

    syncBtn.setName ("syncBtn");
    addAndMakeVisible(syncBtn);
    syncBtn.setAlpha(0.f);
    syncBtn.onClick = [this] { showSyncMenu(); };

	addAndMakeVisible(center.get());
	addAndMakeVisible(depth.get());
	addAndMakeVisible(rate.get());
	addAndMakeVisible(rateSync.get());
	addAndMakeVisible(res.get());
	addAndMakeVisible(morph.get());
	addAndMakeVisible(stereo.get());
	addAndMakeVisible(mix.get());

    rateSync->setVisible(false);

	editor.registerModParam(center.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(depth.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(rate.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(rateSync.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(res.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(morph.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(stereo.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(mix.get(), TetraOPAudioProcessorEditor::kFX);
}

FXPhaser::~FXPhaser()
{
    editor.audioProcessor.params.removeParameterListener(prefix + "sync", this);
}

void FXPhaser::parameterChanged(const juce::String&, float)
{
    juce::MessageManager::callAsync([this] { toggleUIComponents(); });
}

void FXPhaser::onActiveToggle()
{
	center->setEnabled(on);
	depth->setEnabled(on);
	rate->setEnabled(on);
	rateSync->setEnabled(on);
	res->setEnabled(on);
	morph->setEnabled(on);
	stereo->setEnabled(on);
	mix->setEnabled(on);
}

void FXPhaser::paint(juce::Graphics& g)
{
	UIFX::paint(g);
    int sync = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "sync")->load();

    if (sync == 0)
        UIUtils::drawClock(g, syncBtn.getBounds().reduced(4).toFloat(), COLOR_KNOB_LABEL());
    else
        UIUtils::drawNote(g, syncBtn.getBounds().reduced(4).toFloat(), sync - 1, COLOR_KNOB_LABEL());

}

void FXPhaser::toggleUIComponents()
{
    int sync = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "sync")->load();
    rate->setVisible(sync == 0);
    rateSync->setVisible(sync != 0);
    repaint();
}

void FXPhaser::resized()
{
	UIFX::resized();

	mix->setBounds(Rectangle<int>(KNOB_WIDTH, KNOB_HEIGHT).withX(KNOB_WIDTH).withBottomY(getBottom() - 10 - PANEL_PAD));
	res->setBounds(mix->getBounds().translated(0, -KNOB_HEIGHT));
	morph->setBounds(res->getBounds().translated(-KNOB_WIDTH, 0));
	center->setBounds(morph->getBounds().translated(0, -KNOB_HEIGHT));
	depth->setBounds(res->getBounds().translated(0, -KNOB_HEIGHT));
	rate->setBounds(depth->getBounds().translated(0, -KNOB_HEIGHT));
	rateSync->setBounds(rate->getBounds());

	syncBtn.setBounds(Rectangle<int>(25, 25).withX(center->getX() + 15).withY(rate->getY() + 23));
	toggleUIComponents();
}

void FXPhaser::showSyncMenu()
{
    auto mode = (int)editor.audioProcessor.params.getRawParameterValue(prefix + "sync")->load();

    juce::PopupMenu menu;
    menu.addItem(1, "Seconds", true, mode == 0);
    menu.addItem(2, "Straight", true, mode == 1);
    menu.addItem(3, "Triplet", true, mode == 2);
    menu.addItem(4, "Dotted", true, mode == 3);

    auto menuPos = localPointToGlobal(syncBtn.getBounds().getBottomLeft());
    menu.setLookAndFeel(&getLookAndFeel());
    menu.showMenuAsync(juce::PopupMenu::Options()
        .withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
        .withTargetComponent(*this)
        .withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
        [this](int result) {
            if (result == 0) return;
            auto param = editor.audioProcessor.params.getParameter(prefix + "sync");
            param->setValueNotifyingHost(param->convertTo0to1((float)(result - 1)));
        });
}

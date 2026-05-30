#include "FXReverb.h"
#include "../../PluginEditor.h"

FXReverb::FXReverb(TetraOPAudioProcessorEditor& e)
	: UIFX(e, FX::Reverb)
{
	editor.audioProcessor.params.addParameterListener(prefix + "mode", this);
	editor.audioProcessor.params.addParameterListener(prefix + "shimmer_mode", this);

	modeBtn.setName ("modeBtn");
	addAndMakeVisible(modeBtn);
	modeBtn.setAlpha(0.f);
	modeBtn.onClick = [this]()
		{
			showModeMenu();
		};

	predel = std::make_unique<Rotary>(editor, prefix + "predel", "PreDel", Rotary::VerbPredelay);
	decay = std::make_unique<Rotary>(editor, prefix + "decay", "Decay", Rotary::Percent);
	size = std::make_unique<Rotary>(editor, prefix + "revsize", "Size", Rotary::Percent);
	damp = std::make_unique<Rotary>(editor, prefix + "damp", "Damp", Rotary::Percent);
	lowpass = std::make_unique<Rotary>(editor, prefix + "lowpass", "LowPass", Rotary::Percent);
	lowpass->invertValue = true;
	earlylate = std::make_unique<Rotary>(editor, prefix + "earlylate", "E/L", Rotary::Percent);
	density = std::make_unique<Rotary>(editor, prefix + "density", "density", Rotary::Percent);
	mix = std::make_unique<Rotary>(editor, prefix + "mix", "Mix", Rotary::Percent);

	addAndMakeVisible(predel.get());
	addAndMakeVisible(decay.get());
	addAndMakeVisible(size.get());
	addAndMakeVisible(damp.get());
	addAndMakeVisible(lowpass.get());
	addAndMakeVisible(earlylate.get());
	addAndMakeVisible(density.get());
	addAndMakeVisible(mix.get());

	editor.registerModParam(predel.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(decay.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(size.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(damp.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(lowpass.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(earlylate.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(density.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(mix.get(), TetraOPAudioProcessorEditor::kFX);

	onActiveToggle();
}

FXReverb::~FXReverb()
{
	editor.audioProcessor.params.removeParameterListener(prefix + "mode", this);
	editor.audioProcessor.params.removeParameterListener(prefix + "shimmer_mode", this);
}

void FXReverb::parameterChanged(const juce::String& parameterID, float newValue)
{
	(void)parameterID;
	(void)newValue;
	juce::MessageManager::callAsync([this]
		{
            toggleUIComponents();
			repaint();
		});
}

void FXReverb::onActiveToggle()
{
	predel->setEnabled(on);
	decay->setEnabled(on);
	size->setEnabled(on);
	damp->setEnabled(on);
	lowpass->setEnabled(on);
	earlylate->setEnabled(on);
	density->setEnabled(on);
	mix->setEnabled(on);
}

void FXReverb::paint(juce::Graphics& g)
{
	UIFX::paint(g);

	auto mode = juce::jlimit(0, 5, juce::roundToInt(editor.audioProcessor.params.getRawParameterValue(prefix + "mode")->load()));
	UIUtils::drawBevel(g, modeBtn.getBounds().toFloat().expanded(0.5f), 3.f, COLOR_BEVEL());
	g.setColour(COLOR_KNOB_LABEL());
	g.setFont(juce::FontOptions(15.f));
    auto text = mode == 0 ? "Nuclear"
        : mode == 1 ? "Solar"
        : mode == 2 ? "Nova"
        : mode == 3 ? "Space"
        : mode == 4 ? "Singular"
        : "MiniVerb";
	g.drawText(text, modeBtn.getBounds(), juce::Justification::centred);
}

void FXReverb::resized()
{
	UIFX::resized();

	mix->setBounds(Rectangle<int>(KNOB_WIDTH, KNOB_HEIGHT).withX(KNOB_WIDTH).withBottomY(getBottom() - 10 - PANEL_PAD));
	predel->setBounds(mix->getBounds().translated(-KNOB_WIDTH, 0));
	earlylate->setBounds(mix->getBounds().translated(0, -KNOB_HEIGHT));
	density->setBounds(earlylate->getBounds().translated(-KNOB_WIDTH, 0));
	lowpass->setBounds(earlylate->getBounds().translated(0, -KNOB_HEIGHT));
	damp->setBounds(density->getBounds().translated(0, -KNOB_HEIGHT));
	size->setBounds(lowpass->getBounds().translated(0, -KNOB_HEIGHT));
	decay->setBounds(damp->getBounds().translated(0, -KNOB_HEIGHT));

	toggleUIComponents();
}

void FXReverb::toggleUIComponents()
{
    //auto mode = juce::jlimit(0, 5, juce::roundToInt(editor.audioProcessor.params.getRawParameterValue(prefix + "mode")->load()));
    //bool isMiniVerb = mode == 5;
}

void FXReverb::showModeMenu ()
{
    auto mode = juce::jlimit(0, 5, juce::roundToInt(editor.audioProcessor.params.getRawParameterValue(prefix + "mode")->load()));

    juce::PopupMenu menu;
    menu.addItem ( 1, "Nuclear"    , true, mode == 0 );
    menu.addItem ( 2, "Solar"      , true, mode == 1 );
    menu.addItem ( 3, "Nova"       , true, mode == 2 );
    menu.addItem ( 4, "Space"      , true, mode == 3 );
    menu.addItem ( 5, "Singularity", true, mode == 4 );
    menu.addItem ( 6, "MiniVerb"   , true, mode == 5 );

    auto menuPos = localPointToGlobal ( modeBtn.getBounds ().getBottomLeft () );
    menu.setLookAndFeel(&getLookAndFeel());
    menu.showMenuAsync ( juce::PopupMenu::Options ()
        .withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
        .withTargetComponent ( *this )
        .withTargetScreenArea ( { menuPos.getX (), menuPos.getY (), 1, 1 } ),
        [ this ] ( int result ) {
        if ( result == 0 ) return;
        if (auto* param = dynamic_cast<juce::AudioParameterInt*>(editor.audioProcessor.params.getParameter(prefix + "mode")))
            param->setValueNotifyingHost(param->convertTo0to1(result - 1.f));
        repaint ();
    } );
}
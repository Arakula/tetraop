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
    damp->invertValue = true;
	lowcut = std::make_unique<Rotary>(editor, prefix + "lowcut", "Lowcut", Rotary::Hz);
	highcut = std::make_unique<Rotary>(editor, prefix + "highcut", "Highcut", Rotary::Hz);
	highcut->invertValue = true;
	moddepth = std::make_unique<Rotary>(editor, prefix + "moddepth", "ModDep", Rotary::Percent);
	modrate = std::make_unique<Rotary>(editor, prefix + "modrate", "ModRate", Rotary::Hz1f);
	mix = std::make_unique<Rotary>(editor, prefix + "mix", "Mix", Rotary::Percent, true);

	predel->setName          ("predel");
	decay->setName           ("decay");
	size->setName            ("size");
	damp->setName            ("damp");
	lowcut->setName          ("lowcut");
	highcut->setName         ("highcut");
	moddepth->setName        ("moddepth");
	modrate->setName         ("modrate");
	mix->setName             ("mix");

	addAndMakeVisible(predel.get());
	addAndMakeVisible(decay.get());
	addAndMakeVisible(size.get());
	addAndMakeVisible(damp.get());
	addAndMakeVisible(lowcut.get());
	addAndMakeVisible(highcut.get());
	addAndMakeVisible(moddepth.get());
	addAndMakeVisible(modrate.get());
	addAndMakeVisible(mix.get());

	editor.registerModParam(predel.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(decay.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(size.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(damp.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(lowcut.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(highcut.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(moddepth.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(modrate.get(), TetraOPAudioProcessorEditor::kFX);
	editor.registerModParam(mix.get(), TetraOPAudioProcessorEditor::kFX);

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
	toggleUIComponents();
}

void FXReverb::toggleUIComponents()
{
    auto mode = juce::jlimit(0, 5, juce::roundToInt(editor.audioProcessor.params.getRawParameterValue(prefix + "mode")->load()));
    bool isMiniVerb = mode == 5;

    size->setVisible(isMiniVerb);
    damp->setVisible(!isMiniVerb);
    modrate->setVisible(isMiniVerb);
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
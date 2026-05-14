#include "ScaledPluginEditor.h"
#include "../PluginEditor.h"
#include "../PluginProcessor.h"

ScaledPluginEditor::ScaledPluginEditor (TetraOPAudioProcessorEditor* editor_)
    : juce::AudioProcessorEditor (editor_->processor)
    , editor (editor_)
{
    juce::ScopedValueSetter svs (inInit, true);

    auto w = editor->getWidth();
    auto h = editor->getHeight();

    setSize (w, h);
    setResizable (true, true);
    constrainer.setSizeLimits (w / 4, h / 4, w * 4, h * 4);
    constrainer.setFixedAspectRatio (double (w) / h);
    setConstrainer (&constrainer);

    addAndMakeVisible (frame);
    frame.addAndMakeVisible (*editor);
    frame.setBounds (getLocalBounds());

    setLookAndFeel (&editor->getLookAndFeel());

    if (auto scale = editor->audioProcessor.scale; scale > 0)
        setSize (int (w * scale), int (h * scale));
}

ScaledPluginEditor::~ScaledPluginEditor()
{
    processor.editorBeingDeleted (this);
    setLookAndFeel (nullptr);
}

void ScaledPluginEditor::updateSize (juce::Component& c)
{
    if (auto spe = c.findParentComponentOfClass<ScaledPluginEditor>())
    {
        auto w = spe->editor->getWidth();
        auto h = spe->editor->getHeight();

        if (auto scale = spe->editor->audioProcessor.scale; scale > 0)
            spe->setSize (int (w * scale), int (h * scale));
    }
}

void ScaledPluginEditor::setScale (float scale)
{
    auto w = editor->getWidth();
    auto h = editor->getHeight();

    setSize (int (w * scale), int (h * scale));

    if (editor)
        editor->audioProcessor.scale = scale;
}

void ScaledPluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void ScaledPluginEditor::resized()
{
    auto scale = std::min (float (getWidth()) / editor->getWidth(), float (getHeight()) / editor->getHeight());
    frame.setTransform (juce::AffineTransform().scale (scale));

    if (!inInit) {
        editor->audioProcessor.scale = scale;
        editor->audioProcessor.saveSettings();
    }
}

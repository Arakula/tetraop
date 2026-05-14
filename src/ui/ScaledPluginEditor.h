#pragma once

#include <JuceHeader.h>

class TetraOPAudioProcessorEditor;

//==============================================================================
/** If you want your plugin editor to scale, just wrap it in this
*/
class ScaledPluginEditor : public juce::AudioProcessorEditor
{
public:
    ScaledPluginEditor (TetraOPAudioProcessorEditor* editor_);
    ~ScaledPluginEditor() override;

    static void updateSize (juce::Component&);

    void setScale (float scale);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Component                                 frame;
    std::unique_ptr<TetraOPAudioProcessorEditor>    editor;
    juce::ComponentBoundsConstrainer                constrainer;
    bool                                            inInit = false;
};

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "functional"
#include <JuceHeader.h>
#include "Globals.h"
#include "PluginProcessor.h"
#include "ui/ModulatedParam.h"
#include "ui/panels/AboutDialog.h"
#include "ui/CustomLookAndFeel.h"
//#include "ui/widgets/Modulator.h"
//#include "ui/widgets/Macro.h"

using namespace globals;

class ResizeCorner : public juce::Component
{
public:
    std::function<void(int, int)> onDrag;
    std::function<void()> onFinish;

    void mouseDrag(const MouseEvent& e) override
    {
        if (onDrag)
            onDrag(e.getDistanceFromDragStartX(), e.getDistanceFromDragStartY());
    }

    void mouseUp(const MouseEvent& e) override
    {
        (void)e;
        if (onFinish)
            onFinish();
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        Path p;
        p.startNewSubPath(b.getBottomLeft());
        p.lineTo(b.getTopRight());
        p.lineTo(b.getBottomRight());
        g.setColour(COLOR_BACKGROUND());
        g.fillPath(p);

        b = b.reduced(3.f);
        Path pp;
        pp.startNewSubPath(b.getBottomLeft());
        pp.lineTo(b.getTopRight());
        pp.lineTo(b.getBottomRight());
        g.setColour(Colours::white.withAlpha(0.3f));
        g.fillPath(pp);
    }
};


class TetraOPAudioProcessorEditor
  : public juce::AudioProcessorEditor
  , private juce::AudioProcessorValueTreeState::Listener
  , private juce::Timer
{
public:
    TetraOPAudioProcessor& audioProcessor;

    TetraOPAudioProcessorEditor (TetraOPAudioProcessor&);
    ~TetraOPAudioProcessorEditor() override;

    void buildUI();
    void rebuild();
    void loadTheme() const;

    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void registerModParam(ModulatedParam* param);
    void unregisterModParam(ModulatedParam* param);
    void startDragDrop(String mod, Component* comp);
    void quickConnect(String paramId);
    void setMouseHoverParam(ModulatedParam* param);
    void selectTab(int tab);
    void showAboutDialog();
    void showParamContextMenu(ModulatedParam* param);
    // void showModContextMenu(Modulator* mod);
    // void showMacroRename(Macro* macro);

    void paint(Graphics& g) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Panels
    std::unique_ptr<AboutDialog> aboutDialog;

    std::unique_ptr<CustomLookAndFeel> customLookAndFeel;
    bool isDragDropModulation = false;
    //std::unique_ptr<DragDropOverlay> dragDropOverlay;

    //std::unique_ptr<FloatInput> floatInputOverlay;
    //std::unique_ptr<TextInput> textInputOverlay;
    std::unique_ptr<ResizeCorner> resizeCorner;

    TooltipWindow tooltipWindow;

    bool resizing = false;
    float resizeStart = 1.f;
    float resizeRatio = 1.f;
private:
    String dragDropModID = "";
    std::map<std::string, ModulatedParam*> modulatedParams;
    std::unique_ptr<MidiKeyboardComponent> keyboardComponent;
    ModulatedParam* mouseHoverParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TetraOPAudioProcessorEditor)
};

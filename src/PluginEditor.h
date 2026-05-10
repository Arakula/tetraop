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
#include "ui/panels/OSCPanel.h"
#include "ui/panels/FilterPanel.h"
#include "ui/panels/AboutDialog.h"
#include "ui/panels/GlobalsPanel.h"
#include "ui/panels/FmMatrixPanel.h"
#include "ui/panels/EnvDisplay.h"
#include "ui/panels/LfoDisplay.h"
#include "ui/CustomLookAndFeel.h"
//#include "ui/widgets/Modulator.h"
//#include "ui/widgets/Macro.h"

using namespace globals;

class TMP
    : public juce::Component
    , private juce::Timer
{
public:
    TetraOPAudioProcessor& audioProcessor;
    TMP(TetraOPAudioProcessor& p) : audioProcessor(p)
    {
        startTimerHz(30);
    };

    void timerCallback() override
    {
        repaint();
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds();
        g.setFont(FontOptions(16.f));
        g.setColour(Colours::white);
        g.drawText(String("CPU ") + String(audioProcessor.synth->getCpuUsage()), b, Justification::centredLeft);
        g.drawText(String("V ") + String(audioProcessor.synth->getNumActiveVoices()), b, Justification::centredRight);
    }
};

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
    void toggleFmMatrix();
    void selectTab(int tab);
    void showAboutDialog();
    void showParamContextMenu(ModulatedParam* param);
    // void showModContextMenu(Modulator* mod);
    // void showMacroRename(Macro* macro);

    void paint(Graphics& g) override;
    void mouseUp(const juce::MouseEvent& e) override;


    // Panels
    std::unique_ptr<OSCPanel> oscA;
    std::unique_ptr<OSCPanel> oscB;
    std::unique_ptr<OSCPanel> oscC;
    std::unique_ptr<OSCPanel> oscD;
    std::unique_ptr<FilterPanel> filter1;
    std::unique_ptr<FilterPanel> filter2;
    std::unique_ptr<GlobalsPanel> globals;
    std::unique_ptr<FmMatrixPanel> fmMatrix;
    std::unique_ptr<EnvDisplay> envelopes;
    std::unique_ptr<LFODisplay> lfos;
    std::unique_ptr<AboutDialog> aboutDialog;

    std::unique_ptr<CustomLookAndFeel> customLookAndFeel;
    bool isDragDropModulation = false;
    //std::unique_ptr<DragDropOverlay> dragDropOverlay;

    //std::unique_ptr<FloatInput> floatInputOverlay;
    //std::unique_ptr<TextInput> textInputOverlay;
    std::unique_ptr<ResizeCorner> resizeCorner;

    TooltipWindow tooltipWindow;
    std::unique_ptr<TMP> tmp;

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

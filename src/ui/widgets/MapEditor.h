#pragma once

#include <JuceHeader.h>
#include <functional>
#include "../UIUtils.h"
#include "../../Globals.h"
#include "../../engine/Modulation.h"
#include "../../dsp/Pattern.h"
#include "./CurveEditor.h"

using namespace globals;
class TetraOPAudioProcessorEditor;

class MapEditor : public juce::Component, private juce::Timer
{
public:
    MapEditor(TetraOPAudioProcessorEditor& e, std::function<void()> onClose);
    ~MapEditor() override;

    void updateConnectionPointsFromPat();
    void timerCallback() override;
    void setConnection(Modulation::Connection conn);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void showGridMenu(bool gridy);
    void showFileMenu();

    Modulation::Connection conn{};
private:
    std::function<void()> onClose;
    TetraOPAudioProcessorEditor& editor;
    Pattern pat{};

    juce::TextButton enableBtn;
    juce::TextButton gridXBtn;
    juce::TextButton gridYBtn;
    juce::TextButton rotLBtn;
    juce::TextButton rotRBtn;
    juce::TextButton sineBtn;
    juce::TextButton fileBtn;
    juce::TextButton closeBtn;
    std::unique_ptr<CurveEditor> display;

    int gridX = 16;
    int gridBackup = 16;
    int gridY = 16;
    float wheelAccum = 0.f;
};
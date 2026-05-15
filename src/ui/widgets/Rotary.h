#pragma once

#include <JuceHeader.h>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../ModulatedParam.h"
#include "../../Globals.h"

class TetraOPAudioProcessorEditor;
using namespace globals;


class Rotary : public ModulatedParam, private juce::AudioProcessorValueTreeState::Listener {
public:
    static constexpr float DEG130 = 130.0f * juce::MathConstants<float>::pi / 180.0f;
    enum Format {
        Hz,
        Hz1f,
        dB,
        gain2dB,
        HzToSeconds,
        Percent,
        Percent1f,
        float1,
        float2,
        float3,
        float2_100,
        millis,
        seconds1f,
        seconds2f,
        secondsMillis,
        secondsSubMillis,
        ABMix,
        FilterLPHP,
        PitchSemis,
        RateTempo,
        DelayTempo,
        TremoloTempo,
        Pan,
        Integer,
        OSCMorphA,
        OSCMorphB,
        OSCMorphC,
        OSCMorphD,
        OSCMorphA2f,
        OSCMorphB2f,
        OSCMorphC2f,
        OSCMorphD2f,
        TimeFactor,
        Choice,
        VerbPredelay
    };
    float radius = KNOB_RADIUS;
    float yoffset = KNOB_YOFFSET;
    float mod_offset = KNOB_MOD_OFFSET;
    float mod_thickness = KNOB_MOD_THICKNESS;
    float mod_value_radius = KNOB_MODVAL_RADIUS;
    float mod_value_offset = KNOB_MODVAL_OFFSET;
    float value_offset = KNOB_VALUE_OFFSET;
    float value_thickness = KNOB_VALUE_THICKNESS;
    float labelSize = 16.f;
    bool drawValue = true;
    bool drawArc = true;
    bool drawTextLabel = true;
    bool invertValue = false;
    bool forceLabelShowValue = false;
    Colour colorValue = COLOR_ACTIVE();
    Colour colorModValue = COLOR_ACTIVE();
    bool isMatrixBtn = false;
    bool isDark = false;
    juce::String name;
    juce::Colour baseColor = juce::Colours::transparentBlack; // overrides base color when other than transparent black
    juce::Colour labelColor = juce::Colours::transparentBlack;

    Rotary(TetraOPAudioProcessorEditor& e, juce::String paramId, juce::String name, Format format, bool isSymmetric = false);
    ~Rotary() override;

    void setParamId(String pid);
    void setModId(String mid) override;
    void setSmall();
    void setDark();
    void setMatrixBtn();

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    Format format;
    std::function<void()> onMouseDown;
    std::function<void()> onMouseUp;
    std::function<void()> onMouseEnter;
    std::function<void()> onMouseExit;

protected:
    TetraOPAudioProcessorEditor& editor;

private:
    bool isSmall = false;
    void drawRotary(Graphics& g, float slider_pos);
    void drawModValue(Graphics& g, float slider_pos) const;
    void drawModRange(Graphics& g, float slider_pos) const;
    void drawLabel(Graphics& g, float slider_val);

    bool editingMod = false;
    bool isSymmetric;

    float pixels_per_percent{100.0f};
    float cur_normed_value{0.0f};
    juce::Point<int> last_mouse_position;
    juce::Point<int> start_mouse_pos;
    bool mouse_down = false;
    bool mouse_hover = false;
};
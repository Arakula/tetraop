#pragma once

#include <JuceHeader.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../ModulatedParam.h"
#include "../../Globals.h"
#include "../UIUtils.h"

using namespace globals;
class TetraOPAudioProcessorEditor;

class HSlider : public ModulatedParam, private juce::AudioProcessorValueTreeState::Listener {
public:
    enum Format {
        dB,
        ABMix,
        Percent,
    };

	juce::Slider slider;

    HSlider(TetraOPAudioProcessorEditor& e, juce::String paramId, juce::String name, Format format, bool isSymmetric, juce::Colour trackColor);
    ~HSlider() override;

    void setModId(juce::String mid) override;
    void setParamId(juce::String pid);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void positionValuePopup();

    void parameterChanged (const juce::String& parameterID, float newValue) override;
	void drawLabel(juce::Graphics& g, float slider_val);

    Format format;
    bool drawBorder = true;
    bool showLabelPrefix = false;
protected:
    juce::String name;
    TetraOPAudioProcessorEditor& editor;

private:
    bool isSymmetric;
    juce::Rectangle<float> track;
    juce::Rectangle<float> handle;
    float pixels_per_percent{100.0f};
    float cur_normed_value{0.0f};
    juce::Point<int> last_mouse_position;
    juce::Point<int> start_mouse_pos;
    bool mouse_down = false;
    float wheelAccum = 0.f;
    std::unique_ptr<juce::Label> valuePopup;
    bool editingMod = false;
    juce::Colour trackColor;
};


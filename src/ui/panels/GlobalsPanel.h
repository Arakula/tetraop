#pragma once

#include <JuceHeader.h>
#include "../../Globals.h"
#include "../widgets/Rotary.h"
#include "../widgets/LayoutPicker.h"
#include "../widgets/ValuePicker.h"
#include "../widgets/PowerCurve.h"

class TetraOPAudioProcessorEditor;
using namespace globals;

class GlobalsPanel
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
{
public:

	std::unique_ptr<LayoutPicker> layout;
    std::unique_ptr<Rotary> time;
    std::unique_ptr<Rotary> pitch;
    std::unique_ptr<Rotary> vel;
    std::unique_ptr<Rotary> glide;
    std::unique_ptr<PowerCurve> glideTension;

    std::unique_ptr<ValuePicker> poly;
    std::unique_ptr<ValuePicker> bend;

    TextButton monoBtn{ "mono" };
    TextButton legatoBtn{ "legato" };

    GlobalsPanel(TetraOPAudioProcessorEditor& e);
    ~GlobalsPanel() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(Graphics& g) override;
    void resized() override;
    void toggleUIComponents();

private:
    TetraOPAudioProcessorEditor& editor;
};
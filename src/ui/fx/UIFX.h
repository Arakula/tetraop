#pragma once
#include <JuceHeader.h>
#include "../UIUtils.h"
#include "../../Globals.h"
#include "../../dsp/fx/FX.h"
#include <functional>

using namespace globals;
class TetraOPAudioProcessorEditor;

class UIFX : public juce::Component
{
public:
	juce::TextButton onBtn;
	bool active = false;
	FX::FXType type;

    UIFX(TetraOPAudioProcessorEditor& e, FX::FXType type);
    ~UIFX() override;

	void paint(juce::Graphics& g) override;
	void resized() override;

	juce::String prefix;

protected:
    TetraOPAudioProcessorEditor& editor;
	juce::String titleOverride;

private:
	juce::String name = "";
	juce::Colour color{};
};

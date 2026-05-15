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
	int minX = 4;
	int maxX = 100000000;

    UIFX(TetraOPAudioProcessorEditor& e, FX::FXType type);
    ~UIFX() override;

	void mouseDown(const MouseEvent& e) override;
	void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent&) override;
	void paint(juce::Graphics& g) override;
	void resized() override;

	juce::String prefix;
	std::function<void(UIFX*)> onDragEnded;
    std::function<void(UIFX*)> onDrag;

protected:
    TetraOPAudioProcessorEditor& editor;
	juce::String titleOverride;

private:
	juce::String name = "";
	juce::Colour color{};
	juce::ComponentDragger dragger;
};

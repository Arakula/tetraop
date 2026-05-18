#pragma once

#include <JuceHeader.h>
#include "../widgets/Rotary.h"
#include "../widgets/WaveDisplay.h"
#include "../widgets/UnisonWidget.h"
#include "../../engine/PhaseDist.h"
#include "../../engine/TablesManager.h"
#include "../widgets/ValuePicker.h"

class TetraOPAudioProcessorEditor;

class OSCPanel
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
	, private juce::Timer
{
public:

    OSCPanel(TetraOPAudioProcessorEditor& e, int oscId);
    ~OSCPanel() override;

	void timerCallback() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(Graphics& g) override;
    void resized() override;
    void toggleUIComponents();
	void onMouseDownMorph() const;
	void onMouseUpMorph() const;
	void showDistortionMenu();
	void showWavetablesMenu();

	TextButton onBtn;
	TextButton distBtn;
	TextButton morphBtn;
	TextButton tableBtn;
	TextButton nextBtn;
	TextButton prevBtn;
	TextButton fixBtn;

	std::unique_ptr<Rotary> level;
	std::unique_ptr<Rotary> pan;
	std::unique_ptr<Rotary> phase;
	std::unique_ptr<Rotary> rand;
	std::unique_ptr<Rotary> morph;
	std::unique_ptr<Rotary> dist;
	std::unique_ptr<Rotary> detune;
	std::unique_ptr<Rotary> blend;
	std::unique_ptr<Rotary> wide;
	std::unique_ptr<Rotary> semis;
	std::unique_ptr<Rotary> cents;
	std::unique_ptr<ValuePicker> feedback;
	std::unique_ptr<ValuePicker> octave;

	std::unique_ptr<UnisonWidget> unison;
	std::unique_ptr<WaveDisplay> waveDisplay;

private:
	Rectangle<float> viewport{};
	int oscId = 0;
    String prefix;
    TetraOPAudioProcessorEditor& editor;
	bool isMouseDownDist = false;
};
#pragma once
#include <JuceHeader.h>
#include "UIFX.h"
#include "../../dsp/SVF.h"
#include "../../Globals.h"
#include "../widgets/Rotary.h"

class TetraOPAudioProcessorEditor;

class FXEQ
	: public UIFX
	, private juce::AudioProcessorValueTreeState::Listener
	, private juce::Timer
{
public:
	std::vector<std::unique_ptr<Rotary>> freqknobs;
	std::vector<std::unique_ptr<Rotary>> qknobs;
	std::vector<std::unique_ptr<Rotary>> gainknobs;

	juce::TextButton bandBtn;

	FXEQ(TetraOPAudioProcessorEditor& e);
	~FXEQ() override;

	void timerCallback() override;
	void parameterChanged(const juce::String& parameterID, float newValue) override;
	void mouseDown(const juce::MouseEvent& e) override;
	void mouseDrag(const juce::MouseEvent& e) override;
	void mouseUp(const juce::MouseEvent& e) override;
	void mouseDoubleClick(const juce::MouseEvent& e) override;
	void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    void paint(juce::Graphics& g) override;
	void resized() override;
	void toggleUIComponents();

	void updateEQCurve();
	void showBandModeMenu();

private:
	int selband = 0;
	int dragband = -1;
	juce::Rectangle<float> viewBounds{};
	std::array<juce::Rectangle<float>, globals::EQ_BANDS> bandBounds{};
	std::array<SVF, globals::EQ_BANDS> bandFilters{};

	bool mouse_down = false;
	float cur_freq_normed_value = 0.f;
	float cur_gain_normed_value = 0.f;
	float cur_q_normed_value = 0.f;
	float wheelAccum = 0.f;
	juce::Point<int> last_mouse_pos;
	juce::Point<int> start_mouse_pos;

	std::vector<float> magPoints;
};

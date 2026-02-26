/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

// disable warnings for Surge Tunnings library
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4244)
    #pragma warning(disable : 4100)
    #pragma warning(disable : 4267)
    #pragma warning(disable : 4456)
    #pragma warning(disable : 4701)
#endif
#include "Tunings.h"
#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#include <JuceHeader.h>
#include <vector>
#include "libMTSClient.h"
#include "engine/Synth.h"


class TetraOPAudioProcessor
    : public juce::AudioProcessor
    , public juce::AudioProcessorValueTreeState::Listener
    , public juce::ValueTree::Listener
{
public:

    float scale = 1.f;

    // synth
    std::unique_ptr<Synth> synth;

    // tunning
    MTSClient* mtsClientPtr;
    String tuningFileName = "";
    String tuningFileString = "";
    String tuningFileFormat = "";
    std::unique_ptr<Tunings::Tuning> tuning;
    String tuningFileDir = ""; // default directory for open tuning file dialog

    std::vector<float> leftBuf;
    std::vector<float> rightBuf;

    // Playhead
    float osrate = 44100.f;
    double beatsPerSecond = 1.0;
    double secondsPerBeat = 0.01;
    double timeInSeconds = 0.0;
    bool playing = false;

    // UI
    int selectedTab = 0;

    juce::AudioProcessorValueTreeState params;
    juce::UndoManager undoManager;

    //==============================================================================
    TetraOPAudioProcessor();
    ~TetraOPAudioProcessor() override;

    //==============================================================================
    bool supportsMPE() const override { return true; }
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void releaseResources() override;
    bool supportsDoublePrecisionProcessing() const override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    // ========================================================================
    int pickVoice(int note);
    inline void handleMIDI(MidiMessage msg);

    // ========================================================================
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processSubBlock(float* bufL, float* bufR, int nsamples, int blockoffset, int blocksize);

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================/
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void loadSettings();
    void saveSettings();
    void resetSettings();
    void loadTuningFile(String path);
    void loadTuningFileString(const std::string& str, const std::string& format);

private:
    juce::ApplicationProperties settings;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TetraOPAudioProcessor)
};

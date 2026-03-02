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
#include "engine/Modulation.h"


class TetraOPAudioProcessor
    : public juce::AudioProcessor
    , public juce::AudioProcessorValueTreeState::Listener
    , public juce::ValueTree::Listener
{
public:
    struct WTable 
    {
        String name;
        float srate;
        gin::Wavetable tables;
    };

    // synth
    std::unique_ptr<Synth> synth;
    std::unique_ptr<Modulation> modulation;
    std::vector<float> leftBuf;
    std::vector<float> rightBuf;
    float velsense = 1.f; // velocity sensitivity
    WTable wavetables[4];

    //
    int polyphony = 32;
    bool mpe_enabled = false;
    bool configsChanged = true;
    juce::CriticalSection dspLock;
    bool blockMissed = false;

    // tunning
    MTSClient* mtsClientPtr;
    String tuningFileName = "";
    String tuningFileString = "";
    String tuningFileFormat = "";
    std::unique_ptr<Tunings::Tuning> tuning;
    String tuningFileDir = ""; // default directory for open tuning file dialog


    // Playhead
    float srate = 88200.f;
    float osrate = 44100.f; // oversampled srate
    float iosrate = 1.f / 44100.f; // inverse oversampled srate
    double beatsPerSecond = 1.0;
    double secondsPerBeat = 0.01;
    double timeInSeconds = 0.0;
    bool playing = false;

    // UI
    float scale = 1.f;
    int selectedTab = 0;
    String displayMod;

    juce::AudioProcessorValueTreeState params;
    juce::UndoManager undoManager;

    //==============================================================================
    TetraOPAudioProcessor();
    ~TetraOPAudioProcessor() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    //==============================================================================
    void reloadWavetables();
    bool loadWaveTable(gin::Wavetable& table, double sr, const juce::MemoryBlock& wav, const juce::String& format, int size) const;

    //==============================================================================
    bool supportsMPE() const override { return true; }
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool supportsDoublePrecisionProcessing() const override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    // ========================================================================
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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

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
#include "engine/TablesManager.h"
#include "engine/UndoMgr.h"
#include "engine/PresetManager.h"
#include "dsp/fx/FX.h"
#include "dsp/fx/Chorus.h"
#include "dsp/fx/Compressor.h"
#include "dsp/fx/Delay.h"
#include "dsp/fx/Distortion.h"
#include "dsp/fx/EQ.h"
#include "dsp/fx/ReverbFX.h"
#include "dsp/fx/PhaserFX.h"
#include "ui/ScaledPluginEditor.h"
// CLAP
#include "clap-juce-extensions/clap-juce-extensions.h"

class TetraOPAudioProcessor
    : public juce::AudioProcessor
    , public juce::AudioProcessorValueTreeState::Listener
    , public juce::ValueTree::Listener
    , public clap_juce_extensions::clap_properties
{
public:
    std::unique_ptr<TablesManager> tablesMgr;
    juce::AudioProcessorValueTreeState params;

    // fxs
    using FXSlots = std::array<std::unique_ptr<FX>, FX::kFXs>;
    FXSlots fxSlots;
    std::vector<FX::FXType> fxOrder;
    std::vector<FX*> fxchain;
    std::unique_ptr<juce::dsp::Oversampling<float>> distoversampler;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    // synth
    std::unique_ptr<Synth> synth;
    std::unique_ptr<Modulation> modulation;
    std::vector<float> leftBuf;
    std::vector<float> rightBuf;
    bool isLoadingPreset = false;

    // Undo
    std::unique_ptr<UndoMgr> undomgr;

    // Presets
    std::unique_ptr<PresetManager> presetmgr;

    // preferences
    String importExportDir = ""; // default dir for import export
    float scale = 1.f;
    bool unboundedMouse = true;

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
    int osfactor = 1;
    float srate = 44100.f;
    float osrate = 88200.f; // oversampled srate
    float iosrate = 1.f / 44100.f; // inverse oversampled srate
    double beatsPerSecond = 1.0;
    double secondsPerBeat = 0.5;
    double timeInSeconds = 0.0;
    bool playing = false;
    int currBlockSize = 128;
    int currBlockPos = 0;
    int samplesPerBlock = 512;

    // UI
    int selectedTab = 0;
    bool fmMatrixVisible = false;
    bool showRMMatrix = false;
    String displayEnv = "env1";
    String displayLfo = "lfo1";
    std::atomic<float> rmsL;
    std::atomic<float> rmsR;
    std::atomic<bool> FXDirty;
    std::atomic<float> compReduction = 0.f;

    juce::UndoManager undoManager;

    //==============================================================================
    TetraOPAudioProcessor();
    ~TetraOPAudioProcessor() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    //==============================================================================
    void clearAll();
    void sortFX(std::vector <FX::FXType> sorted);
    void onFXChanged();
    std::vector<FX::FXType> normalizeFXOrder(std::vector<FX::FXType> order) const;
    std::unique_ptr<FX> createFX(FX::FXType type);
    void rebuildFXChain();
    void ensureFXExists(FX::FXType type);
    void destroyFX(FX::FXType type);

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
    void processFx(float* bufL, float* bufR, int nsamples);

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

    //== CLAP ======================================================================
    String getWrapperTypeString() {
        if (wrapperType == wrapperType_Undefined && is_clap)
            return "CLAP";

        return juce::AudioProcessor::getWrapperTypeDescription(wrapperType);
    }

private:
    juce::ApplicationProperties settings;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TetraOPAudioProcessor)
};

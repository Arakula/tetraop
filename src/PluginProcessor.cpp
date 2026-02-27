// Copyright 2026 tilr

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Globals.h"

class MetaParameterBool : public juce::AudioParameterBool
{
public:
    using AudioParameterBool::AudioParameterBool;
    bool isMetaParameter() const override { return true; }
};

class MetaParameterChoice : public juce::AudioParameterChoice
{
public:
    using AudioParameterChoice::AudioParameterChoice;
    bool isMetaParameter() const override { return true; }
};

class MetaParameterFloat : public juce::AudioParameterFloat
{
public:
    using AudioParameterFloat::AudioParameterFloat;
    bool isMetaParameter() const override { return true; }
};

class MetaParameterInt : public juce::AudioParameterInt
{
public:
    using AudioParameterInt::AudioParameterInt;
    bool isMetaParameter() const override { return true; }
};

static AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<AudioParameterInt>("polyphony", "Polyphony", 2, 32, 32));
    layout.add(std::make_unique<AudioParameterBool>("legato", "Legato", false));
    layout.add(std::make_unique<AudioParameterBool>("portamento", "Portamento", false));
    layout.add(std::make_unique<AudioParameterBool>("mono", "Mono", false));
    layout.add(std::make_unique<AudioParameterBool>("mpe", "MPE", false));
    layout.add(std::make_unique<AudioParameterInt>("maxblocksize", "Max Blocksize", 0, 8, 7));
    layout.add(std::make_unique<AudioParameterFloat>("glide", "Glide", NormalisableRange<float>(0.0f, 8000.0f, 1.f, 0.5f), 0.f));

    layout.add(std::make_unique<AudioParameterChoice>("layout", "FM Layout", StringArray{ "A_B_C_D", "DCBA", "DC_BA", "DC_B_A", "DA_DB_DC", "BA_CA_DA", "A_CB_DC", "DC_CA_CB", "DC_DB_BA_CA", "DB_CB_BA" }, 0));

    for (int i = 0; i < MAX_ENVELOPES; ++i) {
        layout.add(std::make_unique<AudioParameterChoice>("env" + juce::String(i + 1) + "_mode", "Env" + juce::String(i + 1) + " Mode", StringArray{ "ADSR", "AHD", "DADSR", "DAHDSR" }, 0));
        layout.add(std::make_unique<AudioParameterFloat>("env" + juce::String(i + 1) + "_del", "Env" + juce::String(i + 1) + " Delay", NormalisableRange<float>(0.0f, 5.f, 0.00001f, 0.3f), 0.f));
        layout.add(std::make_unique<AudioParameterFloat>("env" + juce::String(i + 1) + "_att", "Env" + juce::String(i + 1) + " Attack", NormalisableRange<float>(0.0f, 30.f, 0.00001f, 0.3f), .1f));
        layout.add(std::make_unique<AudioParameterFloat>("env" + juce::String(i + 1) + "_hld", "Env" + juce::String(i + 1) + " Hold", NormalisableRange<float>(0.0f, 5.f, 0.00001f, 0.3f), 0.f));
        layout.add(std::make_unique<AudioParameterFloat>("env" + juce::String(i + 1) + "_dec", "Env" + juce::String(i + 1) + " Decay", NormalisableRange<float>(0.0f, 30.f, 0.00001f, 0.3f), 0.15f));
        layout.add(std::make_unique<AudioParameterFloat>("env" + juce::String(i + 1) + "_sus", "Env" + juce::String(i + 1) + " Sustain", NormalisableRange<float>(0.0f, 1.0f), 0.8f));
        layout.add(std::make_unique<AudioParameterFloat>("env" + juce::String(i + 1) + "_rel", "Env" + juce::String(i + 1) + " Release", NormalisableRange<float>(0.001f, 30.f, 0.000001f, 0.3f), .5f));
        layout.add(std::make_unique<AudioParameterFloat>("env" + juce::String(i + 1) + "_tenatt", "Env" + juce::String(i + 1) + " Attack Tension", NormalisableRange<float>(-1.0f, 1.f), 0.0f));
        layout.add(std::make_unique<AudioParameterFloat>("env" + juce::String(i + 1) + "_tendec", "Env" + juce::String(i + 1) + " Decay Tension", NormalisableRange<float>(-1.0f, 1.f), -0.25f));
        layout.add(std::make_unique<AudioParameterFloat>("env" + juce::String(i + 1) + "_tenrel", "Env" + juce::String(i + 1) + " Release Tension", NormalisableRange<float>(-1.0f, 1.f), -0.25f));
    }

    layout.add(std::make_unique<AudioParameterInt>("lfo_grid", "LFO Grid", 2, 16, 8));
    layout.add(std::make_unique<AudioParameterBool>("lfo_grid_snap", "LFO Grid Snap", false));

    for (int i = 0; i < MAX_LFOS; ++i) {
        layout.add(std::make_unique<AudioParameterChoice>("lfo" + juce::String(i + 1) + "_mode", "LFO" + juce::String(i + 1) + " Sync", StringArray{ "Trigger", "Sync", "Envelope" }, 0));
        layout.add(std::make_unique<AudioParameterChoice>("lfo" + juce::String(i + 1) + "_sync", "LFO" + juce::String(i + 1) + " Sync", StringArray{ "RateHz", "Straight", "Tripplet", "Dotted" }, 1));
        layout.add(std::make_unique<AudioParameterFloat>("lfo" + juce::String(i + 1) + "_rate", "LFO" + juce::String(i + 1) + " Rate", NormalisableRange<float>(0.01f, 50.f, 0.00001f, 0.3f), 1.f));
        layout.add(std::make_unique<AudioParameterInt>("lfo" + juce::String(i + 1) + "_rate_sync", "LFO" + juce::String(i + 1) + " Rate Sync", 0, 10, 5)); // 16bar, 2bar, 1bar ... 1/64
        layout.add(std::make_unique<AudioParameterFloat>("lfo" + juce::String(i + 1) + "_smooth", "LFO" + juce::String(i + 1) + " Smooth", 0.f, 1.f, 0.f));
        layout.add(std::make_unique<AudioParameterFloat>("lfo" + juce::String(i + 1) + "_delay", "LFO" + juce::String(i + 1) + " Delay", 0.f, 4.f, 0.f));
        layout.add(std::make_unique<AudioParameterInt>("lfo" + juce::String(i + 1) + "_delay_sync", "LFO" + juce::String(i + 1) + " Delay Sync", 0, 11, 0)); // Off 1/64 1/32 1/16 1/8 ... 16Bar
        layout.add(std::make_unique<AudioParameterFloat>("lfo" + juce::String(i + 1) + "_rise", "LFO" + juce::String(i + 1) + " Rise", 0.f, 4.f, 0.f));
        layout.add(std::make_unique<AudioParameterInt>("lfo" + juce::String(i + 1) + "_rise_sync", "LFO" + juce::String(i + 1) + " Rise Sync", 0, 11, 0)); // Off 1/64 1/32 1/16 1/8 ... 16Bar
    }

    for (int i = 0; i < MAX_RNDS; ++i) {
        auto prefix = String("rnd") + String(i + 1);
        layout.add(std::make_unique<AudioParameterChoice>(prefix + "_mode", "Rand" + String(i + 1) + " Mode", StringArray{ "Perlin", "Hold" }, 0));
        layout.add(std::make_unique<AudioParameterBool>(prefix + "_global", "Rand" + String(i + 1) + " Global Sync", false));
        layout.add(std::make_unique<AudioParameterChoice>(prefix + "_sync", "Rand" + juce::String(i + 1) + " Sync", StringArray{ "RateHz", "Straight", "Tripplet", "Dotted" }, 1));
        layout.add(std::make_unique<AudioParameterFloat>(prefix + "_rate", "Rand" + juce::String(i + 1) + " Rate", NormalisableRange<float>(0.01f, 50.f, 0.00001f, 0.3f), 1.f));
        layout.add(std::make_unique<AudioParameterInt>(prefix + "_rate_sync", "Rand" + juce::String(i + 1) + " Rate Sync", 0, 10, 5)); // 16bar, 2bar, 1bar ... 1/64
    }

    for (int i = 0; i < MAX_MODULATIONS; ++i) {
        layout.add(std::make_unique<AudioParameterFloat>("mod" + juce::String(i + 1) + "_amt", "Mod" + juce::String(i + 1) + " Amount", NormalisableRange<float>(-1.f, 1.f, 0.00001f), 0.5f));
        layout.add(std::make_unique<AudioParameterFloat>("mod" + juce::String(i + 1) + "_ten", "Mod" + juce::String(i + 1) + " Curve", -1.f, 1.f, 0.0f));
    }

    for (int i = 0; i < MAX_MACROS; ++i) {
        layout.add(std::make_unique<AudioParameterFloat>("macro" + juce::String(i + 1), "Macro" + juce::String(i + 1), 0.0f, 1.0f, 0.0f));
    }

    return layout;
}


TetraOPAudioProcessor::TetraOPAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
    )
    , settings{}
    , params(*this, &undoManager, "PARAMETERS", createParameterLayout()),
    mtsClientPtr{nullptr}
#endif
{
    juce::PropertiesFile::Options options{};
    options.applicationName = ProjectInfo::projectName;
    options.filenameSuffix = ".settings";
#if defined(JUCE_LINUX) || defined(JUCE_BSD)
    options.folderName = "~/.config/TetraOP";
#elif defined(JUCE_MAC) || defined(JUCE_IOS)
    options.folderName = "TetraOP";
#endif
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = PropertiesFile::storeAsXML;
    settings.setStorageParameters(options);

    String presetsFolder = options
        .getDefaultFile()
        .getParentDirectory()
        .getChildFile("presets")
        .getFullPathName();

    synth = std::make_unique<Synth>(*this);
    modulation = std::make_unique<Modulation>(*this);

    params.addParameterListener("polyphony", this);
    params.addParameterListener("legato", this);
    params.addParameterListener("mono", this);
    params.addParameterListener("mpe", this);
    params.addParameterListener("maxblocksize", this);

    loadSettings();
}

TetraOPAudioProcessor::~TetraOPAudioProcessor()
{
    MTS_DeregisterClient(mtsClientPtr);
}

void TetraOPAudioProcessor::parameterChanged(const juce::String& paramId, float value)
{
    (void)paramId;
    (void)value;

    configsChanged = true;
}

void TetraOPAudioProcessor::loadSettings ()
{
    if (auto* file = settings.getUserSettings()) {
        scale = (float)file->getDoubleValue("scale", 1.f);
        tuningFileName = file->getValue("tuningFileName", "");
        tuningFileFormat = file->getValue("tuningFileFormat", "");
        tuningFileString = file->getValue("tuningFileString", "");
        tuningFileDir = file->getValue("tuningFileDir", "");
    }
}

void TetraOPAudioProcessor::saveSettings ()
{
    if (auto* file = settings.getUserSettings()) {
        file->setValue("version", PROJECT_VERSION);
        file->setValue("scale", scale);
        file->setValue("tuningFileName", tuningFileName);
        file->setValue("tuningFileFormat", tuningFileFormat);
        file->setValue("tuningFileString", tuningFileString);
        file->setValue("tuningFileDir", tuningFileDir);
    }
    settings.saveIfNeeded();
}

void TetraOPAudioProcessor::resetSettings()
{
    scale = 1.f;
    saveSettings();
}

void TetraOPAudioProcessor::loadTuningFile(String path)
{
    tuning.reset();

    if (path.isEmpty()) {
        tuningFileName = "";
        tuningFileFormat = "";
        tuningFileString = "";
        saveSettings();
        return;
    }

    try {
        String ext = File(path).getFileExtension();
        if (ext == ".scl") {
            auto scl = Tunings::readSCLFile(path.toStdString());
            tuning = std::make_unique<Tunings::Tuning>(Tunings::Tuning(scl));
            tuningFileString = String(scl.rawText);
        }
        else if (ext == ".kbm") {
            auto scl = Tunings::readKBMFile(path.toStdString());
            tuning = std::make_unique<Tunings::Tuning>(Tunings::Tuning(scl));
            tuningFileString = String(scl.rawText);
        }
        else {
            throw "Unkown format";
        }

        tuningFileName = File(path).getFileName();
        tuningFileFormat = File(path).getFileExtension();
        tuningFileDir = File(path).getParentDirectory().getFullPathName();
    }
    catch (...) {
        tuningFileName = "";
        tuningFileString = "";
        tuningFileFormat = "";
    }

    saveSettings();
}

void TetraOPAudioProcessor::loadTuningFileString(const std::string& str, const std::string& format)
{
    tuning.reset();
    try {
        if (format == ".scl") {
            std::istringstream stream(str);
            auto scl = Tunings::readSCLStream(stream);
            tuning = std::make_unique<Tunings::Tuning>(Tunings::Tuning(scl));
        }
        else if (format == ".kbm") {
            std::istringstream stream(str);
            auto scl = Tunings::readKBMStream(stream);
            tuning = std::make_unique<Tunings::Tuning>(Tunings::Tuning(scl));
        }
    }
    catch (...) {}
}

//==============================================================================
const juce::String TetraOPAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TetraOPAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TetraOPAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TetraOPAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TetraOPAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TetraOPAudioProcessor::getNumPrograms()
{
    return 1;
}

int TetraOPAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TetraOPAudioProcessor::setCurrentProgram (int index)
{
    (void)index;
}

const juce::String TetraOPAudioProcessor::getProgramName (int index)
{
    (void)index;
    return {};
}

void TetraOPAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    (void)index;
    (void)newName;
}

//==============================================================================
void TetraOPAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    (void)samplesPerBlock;
    srate = (float)sampleRate;
    osrate = (float)sampleRate;
    synth->setCurrentPlaybackSampleRate(sampleRate);
    synth->prepare();
}

void TetraOPAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TetraOPAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
   #endif

    return true;
  #endif
}
#endif

bool TetraOPAudioProcessor::supportsDoublePrecisionProcessing() const
{
    return false;
}

void TetraOPAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals disableDenormals;

    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    if (totalNumOutputChannels == 0 || numSamples == 0) {
        buffer.clear();
        return;
    }

    if (auto* phead = getPlayHead()) {
        if (auto pos = phead->getPosition()) {
            if (auto tempo = pos->getBpm()) {
                beatsPerSecond = *tempo / 60.0;
                secondsPerBeat = 60.0 / *tempo;
            }
            if (auto seconds = pos->getTimeInSeconds()) {
                timeInSeconds = *seconds;
            }
            else if (auto ppq = pos->getPpqPosition()) {
                if (auto tempo = pos->getBpm()) {
                    timeInSeconds = *ppq * (60.0 / *tempo); // fallback
                }
            }
            playing = pos->getIsPlaying();
        }
    }

    buffer.clear();

    if (configsChanged)
    {
        polyphony = (int)params.getRawParameterValue("polyphony")->load();
        mpe_enabled = (bool)params.getRawParameterValue("mpe")->load();
        synth->setMono((bool)params.getRawParameterValue("mono")->load());
        synth->setLegato((bool)params.getRawParameterValue("legato")->load());
        synth->setPortamento((bool)params.getRawParameterValue("portamento")->load());
        synth->setGlideRate(params.getRawParameterValue("glide")->load());
        synth->setMPE(mpe_enabled);
        synth->setNumVoices(polyphony);
        configsChanged = false;
    }

    int pos = 0;
    int todo = buffer.getNumSamples();

    synth->startBlock();
    while (todo > 0)
    {
        int thisBlock = std::min(todo, 128);
        modulation->tick((double)osrate, thisBlock, (float)secondsPerBeat);
        synth->renderNextBlock(buffer, midiMessages, pos, thisBlock);
        pos += thisBlock;
        todo -= thisBlock;
    }
    synth->endBlock(numSamples);
}

//==============================================================================
bool TetraOPAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* TetraOPAudioProcessor::createEditor()
{
    return new TetraOPAudioProcessorEditor (*this);
}

//==============================================================================
void TetraOPAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = ValueTree("TETRAOP");
    auto paramsState = params.copyState();
    state.appendChild(paramsState, nullptr);
    state.setProperty("version", PROJECT_VERSION, nullptr);

    auto lfos = ValueTree("LFOS");
    for (int i = 0; i < MAX_LFOS; ++i) {
        auto lfo = ValueTree(String("LFO" + String(i + 1)));
        lfo.setProperty("points", var(modulation->lfos[i].pattern.serialize()), nullptr);
        lfos.appendChild(lfo, nullptr);
    }
    state.appendChild(lfos, nullptr);
    state.appendChild(modulation->serialize(), nullptr);

    auto macros = ValueTree("MACROS");
    for (int i = 0; i < MAX_MACROS; ++i) {
        macros.setProperty(String("macro") + String(i + 1), modulation->macroNames[i], nullptr);
    }
    state.appendChild(macros, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TetraOPAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement>xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState == nullptr) { // Fallback to utf8 parsing
        auto xmlString = juce::String::fromUTF8(static_cast<const char*>(data), sizeInBytes);
        xmlState = juce::parseXML(xmlString);
    }

    if (!xmlState) return;
    auto state = ValueTree::fromXml(*xmlState);
    if (!state.isValid()) return;

    auto parameters = state.getChildWithName("PARAMETERS");
    auto lfos = state.getChildWithName("LFOS");
    auto mods = state.getChildWithName("MODULATIONS");
    auto macros = state.getChildWithName("MACROS");

    if (parameters.isValid()) {
        params.replaceState(parameters);
    }

    if (lfos.isValid()) {
        for (int i = 0; i < MAX_LFOS; ++i) {
            auto lfo = lfos.getChildWithName("LFO" + String(i + 1));
            if (lfo.isValid()) {
                auto points = lfo.getProperty("points", "").toString().toStdString();
                if (!points.empty()) {
                    modulation->lfos[i].pattern.unserialize(points);
                }
            }
        }
    }

    if (mods.isValid()) {
        modulation->unserialize(mods);
    }

    for (int i = 0; i < MAX_MACROS; ++i) {
        modulation->macroNames[i] = String("Macro") + String(i + 1);
    }
    if (macros.isValid()) {
        for (int i = 0; i < MAX_MACROS; ++i) {
            String name = macros.getProperty("macro" + String(i + 1)).toString();
            if (name.isNotEmpty()) {
                modulation->macroNames[i] = name;
            }
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TetraOPAudioProcessor();
}

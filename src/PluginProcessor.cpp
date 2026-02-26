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

    loadSettings();
}

TetraOPAudioProcessor::~TetraOPAudioProcessor()
{
    MTS_DeregisterClient(mtsClientPtr);
}

void TetraOPAudioProcessor::parameterChanged(const juce::String& paramId, float value)
{
    (void)value;
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
    osrate = (float)sampleRate;
    synth->setCurrentPlaybackSampleRate(sampleRate);
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

// O(N) version of
// Sort the array by release time if the note is not pressed, otherwise sort by press time.
// The release time sort should take priority over press time sort.
/* int TetraOPAudioProcessor::pickVoice(int note) {
    bool reuseVoices = true;

    // Priority 1: note already playing in a voice
    if (reuseVoices) {
        for (int i = 0; i < polyphony; ++i) {
            if (voices[i]->note == note) {
                return i;
            }
        }
    }

    int pick = 0;
    for (int i = 1; i < polyphony; ++i) {
        const auto& v1 = voices[i];
        const auto& v2 = voices[pick];

        // Priority 2: Released voices come before pressed ones
        if (!v1->pressed && v2->pressed) {
            pick = i;
        }
        else if (v1->pressed && !v2->pressed) {
            // keep current pick (v2 is released, which has priority)
        }
        // Priority 3: Among released voices, pick oldest release
        else if (!v1->pressed && !v2->pressed) {
            if (v1->release_ts < v2->release_ts) pick = i;
        }
        // Priority 4: Among pressed voices, pick oldest press
        else if (v1->pressed && v2->pressed) {
            if (v1->pressed_ts < v2->pressed_ts) pick = i;
        }
    }

    return pick;
} */

/*
void TetraOPAudioProcessor::onNote(MidiMessage msg)
{
    auto note = msg.getNoteNumber();
    int nvoice = pickVoice(note);
    Voice& voice = *voices[nvoice];

    auto useFadeOut = (bool)params.getRawParameterValue("set_fadeout_reused_voices")->load();
    auto legato = (bool)params.getRawParameterValue("legato")->load();
    if (useFadeOut && (!legato || polyphony > 1)) {
        if (!voice.fadedOut) {
            voice.fadeOut();
            midiQueue.push_back({ msg, int(0.001f * NOTE_FADEOUT_MS * srate / oversampling) + blkoffset });
            fadingNotes.add(note);
            return;
        }
        fadingNotes.removeAllInstancesOf(note);
    }

    // remove new notes from sustain pedal notes,
    // fixes notes pressed twice and held should not be released with the pedal
    sustainedNotes.erase(std::remove(sustainedNotes.begin(), sustainedNotes.end(), msg.getNoteNumber()),
        sustainedNotes.end());

    // just in case, make sure note is not added twice
    pressedNotes.erase(std::remove(pressedNotes.begin(), pressedNotes.end(), note),
        pressedNotes.end());

    pressedNotes.push_back(note);
    float vel = msg.getFloatVelocity();
    voice.trigger(++note_press_count, note, modulation->velCurve.get_y_at(vel), msg.getChannel());
    modulation->lastVel = vel;
    modulation->lastUsedVoice = nvoice;
    modulation->onVoiceTriggered(nvoice, (float)timeInSeconds, playing);

    auto mpe_channel = msg.getChannel();
    for (auto& v : voices) {
        if (v->mpe_channel == mpe_channel && v->id != voice.id) {
            v->mpe_channel = -1; // only one MPE active voice per channel
        }
    }

    modulation->mpe[mpe_channel].x = .5f; // MPE X is based on pitch bend, set normal to neutral position
    modulation->mpe[mpe_channel].y = 0.f;
    modulation->mpe[mpe_channel].z = 0.f;
    modulation->mpe[mpe_channel].lift = 0.f;
}
*/

/*
void TetraOPAudioProcessor::offNote(MidiMessage msg)
{
    auto note = msg.getNoteNumber();
    bool isLastPressedNote = pressedNotes.size() && note == pressedNotes.back();

    // if fade_reused_voices is on and there is a fading voice with this note
    // postpone the noteoff event so that it triggers after the postponed noteOn
    for (auto& _note : fadingNotes) {
        if (_note == note) {
            midiQueue.push_back({ msg, int(0.001f * NOTE_FADEOUT_MS * srate / oversampling) + blkoffset });
            return;
        }
    }

    pressedNotes.erase(std::remove(pressedNotes.begin(), pressedNotes.end(), note),
        pressedNotes.end());

    // release all notes with this note off
    for (int i = 0; i < polyphony; ++i) {
        Voice& voice = *voices[i];
        if (voice.note == note && !voice.released) {
            voice.release(++note_release_count);
            if (voice.mpe_channel > 1) {
                modulation->mpe[voice.mpe_channel].lift = msg.getFloatVelocity();
            }
        }
    }

    // update latest used voice to the most recent voice still pressed
    Voice* latest = nullptr;
    for (auto& v : voices) {
        if (!v->pressed) continue;
        if (!latest || v->pressed_ts > latest->pressed_ts)
            latest = v.get();
    }
    if (latest) {
        modulation->lastUsedVoice = latest->id;
    }

    // retrigger last pressed note if polyphony is 1
    if (polyphony == 1 && !pressedNotes.empty() && isLastPressedNote) {
        int lastvel = (int)(voices[0]->vel * 127);
        auto pressed = pressedNotes.back();

        pressedNotes.erase(std::remove(pressedNotes.begin(), pressedNotes.end(), pressed),
            pressedNotes.end());

        // HACK - there is a single voice that was just released
        // mark it as pressed results in glide ON from last used frequency
        // avoids complicated logic or passing extra function arguments
        voices[0]->pressed = true;
        onNote(juce::MidiMessage::noteOn(1, pressed, juce::uint8(lastvel)));
    }
}
*/

inline void TetraOPAudioProcessor::handleMIDI(MidiMessage msg)
{
    if (msg.isNoteOn() && msg.getVelocity() > 0) {
        // onNote(msg);
    }
    else if (msg.isNoteOff() || (msg.isNoteOn() && msg.getVelocity() == 0)) {
        // if (sustainPedalDown) {
        //     auto note = msg.getNoteNumber();
        //     if (std::find(sustainedNotes.begin(), sustainedNotes.end(), note) == sustainedNotes.end())
        //         sustainedNotes.push_back(note);
        // }
        // else {
        //     offNote(msg);
        // }
    }
    else if (msg.isAllSoundOff() || msg.isAllNotesOff()) {
        // clearAll();
    }
    else if (msg.isSustainPedalOn()) {
        // modulation->sustainPedalValue = msg.getControllerValue() / 127.f;
        // sustainPedalDown = true;
    }
    else if (msg.isSustainPedalOff()) {
        // modulation->sustainPedalValue = msg.getControllerValue() / 127.f;
        // sustainPedalDown = false;
        // for (int note : sustainedNotes) {
        //     offNote(juce::MidiMessage::noteOff(1, note));
        // }
        // sustainedNotes.clear();
    }
    else if (msg.isChannelPressure()) {
        // if (mpe_enabled) {
        //     modulation->mpe[msg.getChannel()].z = msg.getChannelPressureValue() / 127.f;
        // }
        // else {
        //     modulation->setChannelPressure(msg.getChannelPressureValue() / 127.f);
        // }
    }
    else if (msg.isAftertouch()) {
        // if (!mpe_enabled) {
        //     auto note = msg.getNoteNumber();
        //     auto val = msg.getAfterTouchValue() / 127.f;
        //     for (auto& voice : voices) {
        //         if (voice->note == note) {
        //             modulation->setVoiceAftertouch(voice->id, val);
        //         }
        //     }
        // }
    }
    else if (msg.isControllerOfType(74) /* && mpe_enabled */ ) {
        // auto val = msg.getControllerValue() / 127.f;
        // modulation->mpe[msg.getChannel()].y = val;
    }
    else if (msg.isController()) {
        // int controllerNumber = msg.getControllerNumber();
        // auto val = msg.getControllerValue() / 127.f;

        // if (controllerNumber == 1) { // modhweel
        //     modulation->modwheelValue = val;
        //     params.getParameter("mod_wheel")->setValueNotifyingHost(val);
        // }
        // else if (controllerNumber == 7) {
        //     modulation->volumePedalValue = val;
        // }
        // else if (controllerNumber == 11) {
        //     modulation->expressPedalValue = val;
        // }
        // else if (controllerNumber == 64) {
        //     modulation->sustainPedalValue = val;
        // }
        // else if (controllerNumber == 67) {
        //     modulation->softPedalValue = val;
        // }
    }
    else if (msg.isPitchWheel()) {
        // float normalized = (msg.getPitchWheelValue() - 8192) / 8191.f;
        // if (mpe_enabled && msg.getChannel() > 1) {
        //     modulation->mpe[msg.getChannel()].x = (normalized + 1) / 2.f;
        // }
        // else {
        //     // triggers another pitch bend calc on processBlock for no purpose other than refreshing the GUI
        //     params.getParameter("bend_wheel")->setValueNotifyingHost((normalized + 1) / 2.f);
        //     calcPitchBendFactor(normalized);
        // }
    }
}

void TetraOPAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals disableDenormals;

    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto totalNumInputChannels = getTotalNumInputChannels();
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

    synth->setMPE(false);
    synth->setMono(false);
    synth->setLegato(false);
    synth->setGlissando(false);
    synth->setPortamento(false);
    synth->setGlideRate(250.f);
    synth->setNumVoices(32);

    int pos = 0;
    int todo = buffer.getNumSamples();

    while (todo > 0)
    {
        int thisBlock = std::min(todo, 32);
        synth->renderNextBlock(buffer, midiMessages, pos, thisBlock);
        pos += thisBlock;
        todo -= thisBlock;
    }


    /* leftBuf.resize(numSamples);
    rightBuf.resize(numSamples);
    std::fill(leftBuf.begin(), leftBuf.end(), 0.f);
    std::fill(rightBuf.begin(), rightBuf.end(), 0.f); */

    // audio blocks gets split into subblocks by midi messages
    // this allows block processing and sequential updates on MIDI

    /* int minBlockSize = 32;
    int maxBlockSize = 32; // or larger if you want
    int blkoffset = 0;
    bool strictMinimum = false;

    auto processSubRange = [&](int start, int length)
    {
        int pos = start;
        while (length > 0)
        {
            int subSize = length;

            if (subSize > maxBlockSize)
                subSize = maxBlockSize;

            if ((pos != 0 || strictMinimum) && subSize < minBlockSize)
                subSize = std::min(length, minBlockSize);

            processSubBlock(leftBuf.data(), rightBuf.data(), subSize, pos, numSamples);

            pos += subSize;
            length -= subSize;
        }
    };

    // iterate over incoming MIDI
    bool firstSubBlock = true;
    int prevSample = 0;
    for (const auto& msg : midiMessages)
    {
        const int midiPos = msg.samplePosition;
        if (midiPos >= numSamples)
            break;

        const bool smallBlockAllowed = (prevSample == 0 && !strictMinimum);
        const int thisBlockSize = smallBlockAllowed ? 1 : minBlockSize;

        // only split if we are beyond minimumSubBlockSize from previous split
        if (midiPos >= prevSample + thisBlockSize)
        {
            // process audio up to this midi msg position
            processSubRange(prevSample, midiPos - prevSample);
            prevSample = midiPos;
        }

        handleMIDI(msg.getMessage());
    } */

    // process any remaining samples after the last MIDI
    /* if (prevSample < numSamples)
        processSubRange(prevSample, numSamples - prevSample); */

    /* for (int ch = 0; ch < totalNumOutputChannels; ++ch)
    {
        auto* src   = (ch % 2 == 0) ? leftBuf.data() : rightBuf.data();
        auto* dst = buffer.getWritePointer(ch);
        std::copy(src, src + numSamples, dst);
    } */
}

void TetraOPAudioProcessor::processSubBlock(float* bufL, float* bufR, int nsamples, int blockoffset, int blocksize)
{
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
}

void TetraOPAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TetraOPAudioProcessor();
}

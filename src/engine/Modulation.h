// Copyright 2025 Tiagolr

#pragma once
#include "JuceHeader.h"
#include <vector>
#include <string>
#include <atomic>
#include <chrono>
#include "../Globals.h"
#include "../dsp/Envelope.h"
#include "../dsp/Pattern.h"
#include "../dsp/LFO.h"
#include "../dsp/RandGen.h"
#include "Voice.h"

class TetraOPAudioProcessor;
using namespace globals;

class Modulation : public juce::AudioProcessorValueTreeState::Listener
{
public:
	struct Connection
	{
	    juce::String src;
	    juce::String dst;
		bool bipolar = false;
		int id; // slider num
		bool bypass = false;
		float amount = 0.0f; // read only, updated from slider
		float tension = 0.0f; // read only, updated from slider
		float power = 1.0f; // read only, updated from slider
		bool mapped = false;
		juce::String mpoints = "0 0 0 1 0 1 1 0 1 0"; // custom curve
	};
	struct Param
	{
		juce::String id;
		int connections = 0;
		std::atomic<float> norm { 0.f };
    	std::atomic<float> value { 0.f };
		juce::NormalisableRange<float> range{};
	};
	struct Modulator
	{
		float x = 0.0f;
		float x_offset = 0.0f; // phase offset used in sync lfos, for UI display only
		float value = 0.0f;
		float release_y = 0.0f; // used to correctly display the the envelope release
		bool active = false;
		bool released = false;
		int connections = 0;
	};
	struct Aftertouch
	{
		std::array<float, MAX_POLYPHONY> voices{};
		float pressure = 0.0; // channel pressure
	};
	struct MPE
	{
		float x = 0.5f;
		float y = 0.0f;
		float z = 0.0f;
		float lift = 0.0f;
	};

	Modulation(TetraOPAudioProcessor& p);
	~Modulation() {}
	void parameterChanged(const juce::String& paramId, float value) override;

	void tick(double srate, int nsamples, float secondsPerBeat);
	void subTick(); // intrablock update
	void tickConnections();
	void tickMacros();
	void connect(const juce::String& src, const juce::String& dst, int sliderId = 0);
	void disconnect(const juce::String& src, const juce::String& dst);
	void disconnectSelectedMod(const juce::String& dst);
	void changeConnection(const juce::String& src, const juce::String& dst, const juce::String newsrc, const juce::String newdst);
	bool isConnected(const juce::String& param);
	bool isSrcConnected(const juce::String& src);
	float getValue(const juce::String& param, bool useCache = false, int blockOffset = 0, bool audioRate = false, float srate = 0.f);
	float getNorm(const juce::String& param);
	float getPolyValue(const juce::String& param, int voiceId, int blockOffset = 0, bool audioRate = false);
	float getEnvelopeValue(int envid, int voiceId, int blockOffset = 0);
	float getKeyTrackFor(const juce::String& param);
	float getKeyTrackOffsetFor(const juce::String& param, int note);
	void setVoiceAftertouch(int voiceId, float value);
	void setChannelPressure(float value);
	void setSelectedMod(const juce::String& id);
	Connection* getConnection(const juce::String& src, const juce::String& dst);
	void setConnectionBypass(const juce::String& src, const juce::String& dst, bool bypass);
	void setConnectionBipolar(const juce::String& src, const juce::String& dst, bool bipolar);
	void setConnectionMPoints(const juce::String& src, const juce::String& dst, juce::String& mpoints);
	void setConnectionMapped(const juce::String& src, const juce::String& dst, bool mapped);
	std::vector<Connection> getConnections();
	float calculateOffset(std::vector<Connection*> conns, int voiceId = -1, int blockOffset = 0, bool audioRate = false, float srate = 0.f);
	bool isAnyVoiceActive();
	void onVoiceTriggered(int voiceId, float songTimeInSeconds, bool songIsPlaying);

	ValueTree serialize();
	void unserialize(const ValueTree state);
	static juce::String mapToString(Pattern& map);
	static void stringToMap(juce::String points, Pattern& map);

	juce::String selectedMod = "env1";
	std::array<Envelope, MAX_ENVELOPES> envs{};
	std::array<LFO, MAX_LFOS> lfos{};
	std::array<RandGen, MAX_RNDS> rnds{};
	Aftertouch aftertouch;
	std::atomic<bool> UIDirty { false }; // Flag to notify UI when it should redraw
	int lastUsedVoice = 0;
	float blockDelta = 0.f; // last block elapsed time in seconds
	float modwheelValue = 0.f;
	float pitchbendValue = 0.5f;
	float volumePedalValue = 0.f;
	float sustainPedalValue = 0.f;
	float expressPedalValue = 0.f;
	float softPedalValue = 0.f;

	std::array<String, MAX_MACROS> macroNames;

	Pattern velCurve; // velocity mapping curve
	float lastVel = 0.f; // last active voice raw velocity

	std::unordered_map<juce::String, Modulator>modulators;
	std::array<MPE, 17> mpe; // MPE first two channels unused, works for channels 2-16
private:
	Param& getParam(const juce::String& pname);
	double internalClock = 0.f; // keep track of elapsed time in seconds, to offset LFOs phase
	void _disconnect(const juce::String& src, const juce::String& dst);
	void _connect(const juce::String& src, const juce::String& dst, int sliderId = 0, int connIndex = -1);
	float randFromVoiceTimestamp(uint64_t ts);
	std::mutex mtx;
	TetraOPAudioProcessor& audioProcessor;
	std::vector<std::unique_ptr<Connection>> connections;
	std::unordered_map<juce::String, std::vector<Connection*>> sources;
	std::unordered_map<juce::String, std::vector<Connection*>> destinations;
	std::unordered_map<juce::String, Param> params;
	std::array<Pattern, MAX_MODULATIONS + 1> curvemaps{}; // modulation custom curve mappings, 0 is not used
	uint64_t start_ts; // used to seed random generated numbers

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Modulation)
};
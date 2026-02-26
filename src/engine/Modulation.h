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

class Modulation
{
public:
	struct Connection
	{
	    std::string src;
	    std::string dst;
		bool bipolar = false;
		int id; // slider num
		bool bypass = false;
		float amount = 0.0f; // read only, updated from slider
		float tension = 0.0f; // read only, updated from slider
		float power = 1.0f; // read only, updated from slider
		bool mapped = false;
		std::string mpoints = "0 0 0 1 0 1 1 0 1 0"; // custom curve
	};
	struct Param
	{
		std::string id;
		int connections = 0;
		std::atomic<float> norm { 0.f };
    	std::atomic<float> value { 0.f };
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

	void tick(double srate, int nsamples, float secondsPerBeat);
	void subTick(); // intrablock update
	void tickConnections();
	void tickMacros();
	void connect(const std::string& src, const std::string& dst, int sliderId = 0);
	void disconnect(const std::string& src, const std::string& dst);
	void disconnectSelectedMod(const std::string& dst);
	void changeConnection(const std::string& src, const std::string& dst, const std::string newsrc, const std::string newdst);
	bool isConnected(const std::string& param);
	bool isSrcConnected(const std::string& src);
	float getValue(const juce::String& param, bool useCache = false, int blockOffset = 0, bool audioRate = false, float srate = 0.f);
	float getNorm(const std::string& param);
	float getPolyValue(const juce::String& param, int voiceId, int blockOffset = 0, bool audioRate = false);
	float getEnvelopeValue(int envid, int voiceId, int blockOffset = 0);
	float getKeyTrackFor(const std::string& param);
	float getKeyTrackOffsetFor(const std::string& param, int note);
	void setVoiceAftertouch(int voiceId, float value);
	void setChannelPressure(float value);
	void setSelectedMod(const std::string& id);
	Connection* getConnection(const std::string& src, const std::string& dst);
	void setConnectionBypass(const std::string& src, const std::string& dst, bool bypass);
	void setConnectionBipolar(const std::string& src, const std::string& dst, bool bipolar);
	void setConnectionMPoints(const std::string& src, const std::string& dst, std::string& mpoints);
	void setConnectionMapped(const std::string& src, const std::string& dst, bool mapped);
	std::vector<Connection> getConnections();
	float calculateOffset(std::vector<Connection*> conns, int voiceId = -1, int blockOffset = 0, bool audioRate = false, float srate = 0.f);
	bool isAnyVoiceActive();
	void onVoiceTriggered(int voiceId, float songTimeInSeconds, bool songIsPlaying);

	ValueTree serialize();
	void unserialize(const ValueTree state);
	static std::string mapToString(Pattern& map);
	static void stringToMap(std::string points, Pattern& map);

	std::string selectedMod = "env1";
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

	std::unordered_map<std::string, Modulator>modulators;
	std::array<MPE, 17> mpe; // MPE first two channels unused, works for channels 2-16
private:
	double internalClock = 0.f; // keep track of elapsed time in seconds, to offset LFOs phase
	void _disconnect(const std::string& src, const std::string& dst);
	void _connect(const std::string& src, const std::string& dst, int sliderId = 0, int connIndex = -1);
	float randFromVoiceTimestamp(uint64_t ts);
	std::mutex mtx;
	TetraOPAudioProcessor& audioProcessor;
	std::vector<std::unique_ptr<Connection>> connections;
	std::unordered_map<std::string, std::vector<Connection*>> sources;
	std::unordered_map<std::string, std::vector<Connection*>> destinations;
	std::unordered_map<std::string, Param> params;
	std::array<Pattern, MAX_MODULATIONS + 1> curvemaps{}; // modulation custom curve mappings, 0 is not used
	uint64_t start_ts; // used to seed random generated numbers

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Modulation)
};
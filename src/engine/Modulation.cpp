#include "Modulation.h"
#include "../PluginProcessor.h"

Modulation::Modulation(TetraOPAudioProcessor& p)
    : audioProcessor(p)
{
    connections.reserve(MAX_MODULATIONS);
    start_ts = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

    velCurve.insertPoint(0.f, 0.f, -0.225f, 1);
    velCurve.insertPoint(1.f, 1.f, 0.f, 1);
    velCurve.buildSegments();
}

float getRateBeats(int sync, LFO::SyncMode mode)
{
    float qn = 1.f;
    if (sync == 0) qn = 1.f * 16.f * 4.f; // 16bar
    if (sync == 1) qn = 1.f * 16.f * 2.f; // 8bar
    if (sync == 2) qn = 1.f * 16.f; // 4bar
    if (sync == 3) qn = 1.f * 8.f; // 2bar
    if (sync == 4) qn = 1.f * 4.f; // bar
    if (sync == 5) qn = 1.f * 2.f; // 1/2
    if (sync == 6) qn = 1.f / 1.f; // 1/4
    if (sync == 7) qn = 1.f / 2.f; // 1/8
    if (sync == 8) qn = 1.f / 4.f; // 1/16
    if (sync == 9) qn = 1.f / 8.f; // 1/32
    if (sync == 10) qn = 1.f / 16.f; // 1/64

    if (mode == LFO::Tripplet)
        qn *= 2 / 3.f;
    else if (mode == LFO::Dotted)
        qn *= 1.5f;

    return qn;
}

float getDelayBeats(int sync, LFO::SyncMode mode)
{
    if (sync == 0) return 0.f;

    float qn = 1.f;
    if (sync == 1) qn = 1.f / 16.f; // 1/64
    if (sync == 2) qn = 1.f / 8.f; // 1/32
    if (sync == 3) qn = 1.f / 4.f; // 1/16
    if (sync == 4) qn = 1.f / 2.f; // 1/8
    if (sync == 5) qn = 1.f / 1.f; // 1/4
    if (sync == 6) qn = 1.f * 2.f; // 1/2
    if (sync == 7) qn = 1.f * 4.f; // bar
    if (sync == 8) qn = 1.f * 8.f; // 2bar
    if (sync == 9) qn = 1.f * 16.f; // 4bar
    if (sync == 10) qn = 1.f * 16.f * 2.f; // 8bar
    if (sync == 11) qn = 1.f * 16.f * 4.f; // 16bar

    if (mode == LFO::Tripplet)
        qn *= 2 / 3.f;
    else if (mode == LFO::Dotted)
        qn *= 1.5f;

    return qn;
}

// update connection values
void Modulation::tickConnections()
{
    for (auto& conn : connections) {
        auto prefix = juce::String("mod") + juce::String(conn->id) + "_";
        auto amount = getValue((prefix + "amt"), false);
        auto tension = getValue((prefix + "ten"), false);
        conn->amount = amount;
        if (conn->tension != tension) {
            conn->tension = tension;
            conn->power = pow(1.1f, std::fabs(tension * POWER_CURVE_POWER));
        }
    }
}

// macros are both a modulator and destination
void Modulation::tickMacros()
{
    auto lastVoice = (Voice*)audioProcessor.synth->getVoice(lastUsedVoice);
    for (int i = 0; i < MAX_MACROS; ++i) {
        auto id = (String("macro") + String(i + 1)).toStdString();
        auto& mod = modulators[id];
        mod.active = lastVoice->isActive() || lastVoice->pressed;
        mod.value = getValue(id, false);
    }
}

// Modulators, connectors and macros depend on each other
// When a noteOn or noteOff arrives, refresh values
void Modulation::subTick()
{
    std::lock_guard<std::mutex> lock(mtx);
    tickConnections();
    tickMacros();
}

void Modulation::tick(double srate, int nsamples, float secondsPerBeat)
{
    std::lock_guard<std::mutex> lock(mtx);

    // FIX standalone build
    if (secondsPerBeat == 0.f)
        secondsPerBeat = 0.25f;

    // update connections before modulators that depend on them
    tickConnections();

    // init envelopes
    for (auto i = 0; i < MAX_ENVELOPES; ++i) {
        auto prefix = juce::String("env") + juce::String(i + 1) + "_";
        auto mode = (Envelope::Mode)audioProcessor.params.getRawParameterValue(prefix + "mode")->load();
        float delay = getValue((prefix + "del").toStdString());
        float attack = getValue((prefix + "att").toStdString());
        float hold = getValue((prefix + "hld").toStdString());
        float decay = getValue((prefix + "dec").toStdString());
        float sustain = getValue((prefix + "sus").toStdString());
        float release = getValue((prefix + "rel").toStdString());
        float tenatt = getValue((prefix + "tenatt").toStdString());
        float tendec = getValue((prefix + "tendec").toStdString());
        float tenrel = getValue((prefix + "tenrel").toStdString());
        auto& env = envs[i];
        env.init(mode, delay, attack, hold, decay, sustain, release, tenatt, tendec, tenrel);
    }

    // init lfos
    for (auto i = 0; i < MAX_LFOS; ++i) {
        auto& lfo = lfos[i];
        auto prefix = juce::String("lfo") + juce::String(i + 1) + "_";
        auto mode = (LFO::Mode)getValue((prefix + "mode").toStdString());
        auto sync = (LFO::SyncMode)getValue((prefix + "sync").toStdString());

        auto rate = sync == LFO::SyncMode::Rate
            ? getValue((prefix + "rate").toStdString())
            : getRateBeats((int)getValue((prefix + "rate_sync").toStdString()), sync);
        if (sync != LFO::Rate)
            rate *= secondsPerBeat;

        auto delay = sync == LFO::SyncMode::Rate
            ? getValue((prefix + "delay").toStdString())
            : getDelayBeats((LFO::SyncMode)getValue((prefix + "delay_sync").toStdString()), sync);
        if (sync != LFO::Rate)
            delay *= secondsPerBeat;

        auto rise = sync == LFO::SyncMode::Rate
            ? getValue((prefix + "rise").toStdString())
            : getDelayBeats((LFO::SyncMode)getValue((prefix + "rise_sync").toStdString()), sync);
        if (sync != LFO::Rate)
            rise *= secondsPerBeat;

        auto smooth = getValue((prefix + "smooth").toStdString());

        auto duration = rate;
        bool srateChanged = lfo.srate != srate;
        lfo.init(srate, duration, delay, rise, mode);
        if (lfo.smooth != smooth || srateChanged) {
            lfo.setSmooth(smooth * smooth * 0.25f);
        }
    }

    // init rand generators
    for (int i = 0; i < MAX_RNDS; ++i) {
        auto prefix = juce::String("rnd") + juce::String(i + 1) + "_";
        auto mode = (RandGen::Mode)audioProcessor.params.getRawParameterValue(prefix + "mode")->load();
        auto sync = (LFO::SyncMode)audioProcessor.params.getRawParameterValue(prefix + "sync")->load();
        auto global = (LFO::SyncMode)audioProcessor.params.getRawParameterValue(prefix + "global")->load();

        auto rate = sync == LFO::SyncMode::Rate
            ? getValue((prefix + "rate").toStdString())
            : getRateBeats((int)getValue((prefix + "rate_sync").toStdString()), sync);
        if (sync != LFO::Rate)
            rate *= secondsPerBeat;

        auto& rnd = rnds[i];
        rnd.init(rate, mode, global);
    }

    auto lastVoice = (Voice*)audioProcessor.synth->getVoice(lastUsedVoice);
    bool voiceActive = lastVoice->isActive() || lastVoice->pressed;

    blockDelta = (float)(nsamples / srate);
    float dt = blockDelta;
    internalClock += (double)dt;

    // tick envelopes
    for (int i = 0; i < MAX_ENVELOPES; ++i) {
        auto& env = envs[i];
        float value = !lastVoice->pressed && !lastVoice->released
            ? env.getValue(0.f, 0.f)
            : env.getValue(lastVoice->attack_elapsed, lastVoice->release_elapsed, dt, lastVoice->released);

        // store modulator values
        auto& mod = modulators[(String("env") + String(i + 1)).toStdString()];
        mod.value = value;
        mod.released = lastVoice->released;
        mod.x = env.mode == Envelope::AHD
            ? lastVoice->attack_elapsed + lastVoice->release_elapsed + dt
            : lastVoice->released
                ? lastVoice->release_elapsed + dt
                : lastVoice->attack_elapsed + dt;
        mod.release_y = env.lrelgain;
        mod.active = voiceActive || (mod.value > 0.f &&
            (lastVoice->release_elapsed || lastVoice->attack_elapsed));
    }

    // tick LFOS
    for (int i = 0; i < MAX_LFOS; ++i) {
        auto& mod = modulators[(String("lfo") + String(i + 1)).toStdString()];
        auto& lfo = lfos[i];
        auto elapsed = !lastVoice->pressed && !lastVoice->released
            ? 0.f // initial voice state
            : lfo.mode == LFO::Trigger || lfo.mode == LFO::Envelope
            ? lastVoice->attack_elapsed + lastVoice->release_elapsed + dt
            : lfo.voices[0].x + dt; // Sync mode

        float value = lfo.getSmoothedValue(elapsed, 0);
        mod.value = value;
        mod.x = elapsed;
        mod.x_offset = lfo.voices[0].phase_offset;
        mod.active = voiceActive;
    }

    // tick randgens
    for (int i = 0; i < MAX_RNDS; ++i) {
        auto& mod = modulators[(String("rnd") + String(i + 1)).toStdString()];
        auto& rnd = rnds[i];
        auto elapsed = !lastVoice->pressed && !lastVoice->released
            ? 0.f
            : rnd.globalSync ? mod.x + dt
            : lastVoice->attack_elapsed + lastVoice->release_elapsed + dt;

        float value = rnd.getValue(elapsed, rnd.globalSync ? 0 : lastUsedVoice + 1);
        mod.value = value;
        mod.x = elapsed;
        mod.x_offset = rnd.voices[0].phase_offset;
        mod.active = voiceActive;
    }

    // Tick other modulators

    // update velocity modulator
    modulators["vel"].active = voiceActive;
    modulators["vel"].x = lastVoice->vel;
    modulators["vel"].value = lastVoice->vel;

    // update keytrack modulator
    modulators["key"].active = voiceActive;
    modulators["key"].x = lastVoice->key;
    modulators["key"].value = lastVoice->key;

    // update modwheel modulator
    modulators["mod"].active = voiceActive;
    modulators["mod"].value = modwheelValue;

    // update pitchbend modulator
    modulators["bend"].active = voiceActive;
    modulators["bend"].value = pitchbendValue;

    // expression pedal
    modulators["exp"].active = voiceActive;
    modulators["exp"].value = expressPedalValue;

    // volume pedal
    modulators["vol"].active = voiceActive;
    modulators["vol"].value = volumePedalValue;

    // sustain pedal
    modulators["sus"].active = voiceActive;
    modulators["sus"].value = sustainPedalValue;

    // sustain pedal
    modulators["soft"].active = voiceActive;
    modulators["soft"].value = softPedalValue;

    // update rand modulator
    modulators["rand"].active = voiceActive;
    modulators["rand"].value = randFromVoiceTimestamp(lastVoice->pressed_ts);

    // update aftertouch mod
    modulators["at"].active = voiceActive;
    modulators["at"].value = aftertouch.voices[lastVoice->id];

    // update MPE mods
    modulators["x"].active = (voiceActive) && lastVoice->mpe_channel > 1;
    modulators["x"].value = lastVoice->mpe_channel > 1 ? mpe[lastVoice->mpe_channel].x : 0.f;
    modulators["y"].active = (voiceActive) && lastVoice->mpe_channel > 1;
    modulators["y"].value = lastVoice->mpe_channel > 1 ? mpe[lastVoice->mpe_channel].y : 0.f;
    modulators["z"].active = (voiceActive) && lastVoice->mpe_channel > 1;
    modulators["z"].value = lastVoice->mpe_channel > 1 ? mpe[lastVoice->mpe_channel].z : 0.f;
    modulators["lift"].active = (voiceActive) && lastVoice->mpe_channel > 1;
    modulators["lift"].value = (lastVoice->mpe_channel > 1 && lastVoice->released)
        ? mpe[lastVoice->mpe_channel].lift : 0.f;

    // cache each param value based on modulations, used by UI
    for (auto& [dst, conns] : destinations) {
        float offset = calculateOffset(conns);
        auto param = audioProcessor.params.getParameter(dst);
        auto norm = param->getValue();
        auto& p = params[dst];
        p.norm.store(std::clamp(norm + offset, 0.f, 1.f));
        p.value.store(param->convertFrom0to1(p.norm));
    }

    tickMacros();
}

/*
* Public connect method, locks behind a mutex
*/
void Modulation::connect(const std::string& src, const std::string& dst, int sliderId)
{
    if (src == dst) return; // prevent macro1 <> macro1 connections
    disconnect(src, dst);
    std::lock_guard<std::mutex> lock(mtx);
    _connect(src, dst, sliderId);
}

/*
* Inner connect method, no mutex lock
*
* connIndex sets the position in the connections array, used by changeConnection()
*/
void Modulation::_connect(const std::string& src, const std::string& dst, int sliderId, int connIndex)
{
    if (connections.size() == MAX_MODULATIONS) return;

    std::unordered_set<int> usedSliderIds;
    for (auto& conn : connections) {
        usedSliderIds.insert(conn->id);
    }

    if (sliderId > 0 && usedSliderIds.count(sliderId) > 0)
        return;

    if (sliderId == 0) {
        // pick an unused slider
        for (int id = 1; id <= MAX_MODULATIONS; ++id) {
            if (usedSliderIds.find(id) == usedSliderIds.end()) {
                sliderId = id;
                break;
            }
        }

        // reset slider to default values
        auto amt = audioProcessor.params.getParameter(String("mod") + String(sliderId) + "_amt");
        amt->setValueNotifyingHost(amt->getDefaultValue());
        auto ten = audioProcessor.params.getParameter(String("mod") + String(sliderId) + "_ten");
        ten->setValueNotifyingHost(ten->getDefaultValue());
    }

    String ssrc = String(src);
    bool bipolar = ssrc.startsWith("lfo") || ssrc == "key" || ssrc == "x" || ssrc == "bend" || ssrc == "vel";

    auto conn = std::make_unique<Connection>(Connection{ src, dst, bipolar, sliderId, false });

    stringToMap(conn->mpoints, curvemaps[sliderId]); // reset curvemapping
    Connection* ptr = conn.get();
    sources[src].push_back(ptr);
    destinations[dst].push_back(ptr);
    if (connIndex == -1) {
        connections.push_back(std::move(conn));
    }
    else {
        connections.insert(connections.begin() + connIndex, std::move(conn));
    }
    params[dst].connections += 1;
    params[dst].id = dst;
    modulators[src].connections += 1;
    UIDirty.store(true);
}

/*
* Public disconnect method with mutex lock
*/
void Modulation::disconnect(const std::string& src, const std::string& dst)
{
    std::lock_guard<std::mutex> lock(mtx);
    _disconnect(src, dst);
}

/*
* Inner disconect method, no mutex lock
*/
void Modulation::_disconnect(const std::string& src, const std::string& dst)
{
    auto it = std::find_if(connections.begin(), connections.end(),
        [&src, &dst](const std::unique_ptr<Connection>& connPtr) {
            return connPtr->src == src && connPtr->dst == dst;
        });

    if (it != connections.end()) {
        Connection* ptr = it->get();

        auto& srcList = sources[src];
        srcList.erase(std::remove(srcList.begin(), srcList.end(), ptr), srcList.end());
        if (srcList.empty()) sources.erase(src);

        auto& dstList = destinations[dst];
        dstList.erase(std::remove(dstList.begin(), dstList.end(), ptr), dstList.end());
        if (dstList.empty()) destinations.erase(dst);

        params[dst].connections -= 1;
        modulators[src].connections -= 1;
        connections.erase(it);
        UIDirty.store(true);
    }
}

void Modulation::disconnectSelectedMod(const std::string& dst)
{
    if (selectedMod.empty()) return;
    disconnect(selectedMod, dst);
}

bool Modulation::isConnected(const std::string& param)
{
    auto it = params.find(param);
    return it != params.end() && it->second.connections > 0;
}

bool Modulation::isSrcConnected(const std::string& src)
{
    auto it = sources.find(src);
    return it != sources.end() && !it->second.empty();
}

Modulation::Connection* Modulation::getConnection(const std::string& src, const std::string& dst)
{
    auto it = std::find_if(connections.begin(), connections.end(),
        [&src, &dst](const std::unique_ptr<Connection>& connPtr) {
            return connPtr->src == src && connPtr->dst == dst;
        });

    if (it == connections.end())
        return nullptr;

    return it->get();
}

// Change connection source and/or destination
// Replaces the old connection with a new one with same params and same array position
void Modulation::changeConnection(const std::string& src, const std::string& dst, const std::string newsrc, const std::string newdst)
{
    std::lock_guard<std::mutex> lock(mtx);
    if (newsrc == newdst) return; // prevent macro1 <> macro1

    auto it = std::find_if(connections.begin(), connections.end(),
        [&src, &dst](const std::unique_ptr<Connection>& connPtr) {
            return connPtr->src == src && connPtr->dst == dst;
        });
    if (it == connections.end()) return; // previous connection not found
    auto index = std::distance(connections.begin(), it);
    Connection oldconn = *(*it); // create a copy of previous connetction

    if (getConnection(newsrc, newdst))
        return; // new connection already exists

    _disconnect(src, dst);
    _connect(newsrc, newdst, oldconn.id, (int)index);

    auto* newconn = getConnection(newsrc, newdst);
    newconn->amount = oldconn.amount;
    newconn->bipolar = oldconn.bipolar;
    newconn->tension = oldconn.tension;
    newconn->power = oldconn.power;
    newconn->bypass = oldconn.bypass;
    newconn->mapped = oldconn.mapped;
    newconn->mpoints = oldconn.mpoints;

    stringToMap(newconn->mpoints, curvemaps[newconn->id]);
}

void Modulation::setConnectionBypass(const std::string& src, const std::string& dst, bool bypass)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto* conn = getConnection(src, dst);
    if (conn != nullptr) {
        conn->bypass = bypass;
        UIDirty.store(true);
    }
}

void Modulation::setConnectionBipolar(const std::string& src, const std::string& dst, bool bipolar)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto* conn = getConnection(src, dst);
    if (conn != nullptr) {
        conn->bipolar = bipolar;
        UIDirty.store(true);
    }
}

void Modulation::setConnectionMPoints(const std::string& src, const std::string& dst, std::string& mpoints)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto* conn = getConnection(src, dst);
    if (conn != nullptr) {
        conn->mpoints = mpoints;
        stringToMap(mpoints, curvemaps[conn->id]);
        UIDirty.store(true);
    }
}

void Modulation::setConnectionMapped(const std::string& src, const std::string& dst, bool mapped)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto* conn = getConnection(src, dst);
    if (conn != nullptr) {
        conn->mapped = mapped;
        UIDirty.store(true);
    }
}

/**
 * Thread safe get modulated value
 */
float Modulation::getValue(const juce::String& pname, bool useCache, int blockOffset, bool audioRate, float srate)
{
    if (useCache) {
        auto it = params.find(pname.toStdString());
        return it != params.end() && it->second.connections > 0
            ? it->second.value.load()
            : audioProcessor.params.getRawParameterValue(pname)->load();
    }
    else {
        auto it = destinations.find(pname.toStdString());
        if (it != destinations.end()) {
            float offset = calculateOffset(it->second, lastUsedVoice, blockOffset, audioRate, srate);
            auto param = audioProcessor.params.getParameter(pname);
            auto norm = param->getValue();
            return param->convertFrom0to1(std::clamp(norm + offset, 0.f, 1.f));
        }
        return audioProcessor.params.getRawParameterValue(pname)->load();
    }
}

/**
 * Thread safe get modulated norm
 */
float Modulation::getNorm(const std::string& param)
{
    auto it = params.find(param);
    return it != params.end() && it->second.connections > 0
        ? it->second.norm.load()
        : audioProcessor.params.getParameter(param)->getValue();
}

/**
 * Recalculate value for a specific voice
 */
float Modulation::getPolyValue(const juce::String& pname, int voiceId, int blockOffset, bool audioRate)
{
    auto it = destinations.find(pname.toStdString());
    if (it != destinations.end()) {
        float offset = calculateOffset(it->second, voiceId, blockOffset, audioRate);
        auto param = audioProcessor.params.getParameter(pname);
        auto norm = param->getValue();
        return param->convertFrom0to1(std::clamp(norm + offset, 0.f, 1.f));
    }
    return audioProcessor.params.getRawParameterValue(pname)->load();
}

float Modulation::getEnvelopeValue(int envid, int voiceId, int blockOffset)
{
    auto voice = (Voice*)audioProcessor.synth->getVoice(voiceId);
    auto& env = envs[envid];

    auto dt = blockOffset / audioProcessor.osrate;

    return env.getValue(
        voice->attack_elapsed,
        voice->release_elapsed,
        dt,
        voice->released
    );
}

/*
* Calculates the sum of multiple connections contributions
* cons are the connections to a single param from different sources
* voiceId when -1 (default) uses the pre-calculated modulator values from tick()
*
* sampsOffset used in audioRate modulation for elapsed time inside a block
*/
float Modulation::calculateOffset(std::vector<Connection*> conns, int voiceId, int blockOffset, bool audioRate, float srate)
{
    auto calcOffset = [this](float value, Connection* conn)
        {
            if (conn->amount == 0)
                return 0.f;

            if (conn->mapped)
                value = curvemaps[conn->id].get_y_at(value);

            if (conn->bipolar)
                value = value * 2 - 1;

            auto absamt = std::abs(conn->amount);
            value = value * absamt;

            if (conn->tension != 0.f) {
                float sign = value >= 0 ? 1.f : -1.f;
                float absval = std::fabs(value) / absamt; // normalize between 0...1 for pow
                float shaped = conn->tension < 0
                    ? -1 * (std::pow(1 - absval, conn->power) - 1)
                    : std::pow(absval, conn->power);
                value = shaped * sign * absamt;
            }

            if (conn->amount < 0.f) value = -value;
            if (conn->bipolar) value *= 0.5;

            return value;
        };

    float offset = 0.f;
    auto voice = voiceId == -1
        ? nullptr
        : (Voice*)audioProcessor.synth->getVoice(voiceId);

    auto dt = audioRate
        ? blockOffset / (srate > 0.f ? srate : audioProcessor.osrate)
        : blockDelta;

    for (auto* conn : conns) {
        if (conn->bypass) continue;
        juce::String src = conn->src;
        float srcValue = 0.0f;
        int modindex = src.retainCharacters("0123456789").getIntValue();
        auto& mod = modulators[src.toStdString()];

        if (voice == nullptr) { // no voice, global modulator, use cached value
            srcValue = mod.value;
            if (!mod.active)
                continue; // FIX
        }
        else if (src.startsWith("env") && modindex > 0 && modindex <= MAX_ENVELOPES) {
            auto envindex = modindex - 1;
            auto& env = envs[envindex];
            srcValue = env.getValue(
                voice->attack_elapsed,
                voice->release_elapsed,
                dt,
                voice->released
            );
        }
        else if (src.startsWith("lfo") && modindex > 0 && modindex <= MAX_LFOS) {
            auto lfoindex = modindex - 1;
            auto& lfo = lfos[lfoindex];
            auto elapsed = lfo.mode == LFO::Trigger || lfo.mode == LFO::Envelope
                ? voice->attack_elapsed + voice->release_elapsed
                : lfo.voices[0].x;

            if (audioRate) {
                srcValue = lfo.getAudioRateValue(elapsed, dt, voiceId + 1, conn->dst);
            }
            else {
                elapsed += dt;
                srcValue = lfo.getSmoothedValue(elapsed, voiceId + 1);
            }
        }
        else if (src.startsWith("rnd") && modindex > 0 && modindex <= MAX_RNDS) {
            auto rndindex = modindex - 1;
            auto& rnd = rnds[rndindex];
            auto elapsed = rnd.globalSync
                ? mod.x + dt
                : voice->attack_elapsed + voice->release_elapsed + dt;
            srcValue = rnd.getValue(elapsed, rnd.globalSync ? 0 : voiceId + 1);
        }
        else if (src == "mod") {
            srcValue = modwheelValue;
        }
        else if (src == "bend") {
            srcValue = pitchbendValue;
        }
        else if (src == "key") {
            auto dst = String(conn->dst);
            if (dst.endsWith("pitch")) { // keytracking for pitch is implemented in getKeyTrackFor()
                continue; // skip this calc completely
            }
            srcValue = voice->key;
        }
        else if (src == "vel") {
            srcValue = voice->vel;
        }
        else if (src == "rand") {
            srcValue = randFromVoiceTimestamp(voice->pressed_ts);
        }
        else if (src.startsWith("macro")) {
            srcValue = getValue(src.toStdString(), true);
        }
        else if (src == "at") {
            if (audioProcessor.mpe_enabled)
                continue;
            srcValue = aftertouch.voices[voice->id];
        }
        else if (src == "x") {
            if (voice->mpe_channel <= 1 || !audioProcessor.mpe_enabled)
                continue;
            srcValue = mpe[voice->mpe_channel].x;
        }
        else if (src == "y") {
            if (voice->mpe_channel <= 1 || !audioProcessor.mpe_enabled)
                continue;
            srcValue = mpe[voice->mpe_channel].y;
        }
        else if (src == "z") {
            if (voice->mpe_channel <= 1 || !audioProcessor.mpe_enabled)
                continue;
            srcValue = mpe[voice->mpe_channel].z;
        }
        else if (src == "lift") {
            if (voice->mpe_channel <= 1 || !audioProcessor.mpe_enabled || !voice->released)
                continue;
            srcValue = mpe[voice->mpe_channel].lift;
        }
        else if (src == "sus" || src == "exp" || src == "soft" || src == "vol") {
            srcValue = mod.value;
        }
        offset += calcOffset(srcValue, conn);
    }

    return offset;
}

/*
* -1 to reset voice aftertouch to current channel pressure
*/
void Modulation::setVoiceAftertouch(int voiceId, float value)
{
    aftertouch.voices[voiceId] = value < 0.f
        ? aftertouch.pressure
        : value;
}
void Modulation::setChannelPressure(float value)
{
    aftertouch.pressure = value;
    for (int i = 0; i < MAX_POLYPHONY; ++i) {
        aftertouch.voices[i] = value;
    }
}

void Modulation::setSelectedMod(const std::string& id)
{
    selectedMod = id;
    String sid = String(id);
    if (sid.startsWith("env") || sid.startsWith("lfo") || sid.startsWith("rnd")) {
        audioProcessor.displayMod = sid;
    }
    UIDirty.store(true);
}

bool Modulation::isAnyVoiceActive()
{
    auto lastVoice = (Voice*)audioProcessor.synth->getVoice(lastUsedVoice);
    return lastVoice->isActive() || lastVoice->pressed;
}

float Modulation::getKeyTrackFor(const std::string& param)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto* conn = getConnection("key", param);
    if (conn == nullptr || conn->bypass)
        return 0.f;

    return conn->amount;
}

float Modulation::getKeyTrackOffsetFor(const std::string& param, int note)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto* conn = getConnection("key", param);
    if (conn == nullptr || !conn->mapped)
        return note / 127.f;

    return curvemaps[conn->id].get_y_at(note / 127.f);
}

void Modulation::onVoiceTriggered(int voiceId, float songTimeInSeconds, bool songIsPlaying)
{
    std::lock_guard<std::mutex> lock(mtx);
    bool resetGlobalVoice = ((Voice*)audioProcessor.synth->getVoice(lastUsedVoice))->id == voiceId;
    for (int i = 0; i < MAX_LFOS; ++i) {
        auto& mod = modulators[(String("lfo") + String(i + 1)).toStdString()];
        auto& lfo = lfos[i];

        auto phaseOffset = 0.f;
        if (lfo.mode == LFO::Mode::Sync) {
            phaseOffset = mod.active
                ? mod.x_offset + mod.x + lfo.delay // keep the voice LFOs in sync with any running instance
                : songIsPlaying
                ? songTimeInSeconds + lfo.delay // make the first voice running this LFO in sync with song position
                : (float)internalClock; // if the song is not playing use a global timer to start the LFO somewhere
        }

        lfo.trigger(voiceId + 1, phaseOffset);
        if (resetGlobalVoice) {
            lfo.trigger(0, phaseOffset);
        }
    }

    for (int i = 0; i < MAX_RNDS; ++i) {
        auto& mod = modulators[(String("rnd") + String(i + 1)).toStdString()];
        auto& rnd = rnds[i];

        auto phaseOffset = 0.f;
        if (rnd.globalSync) {
            phaseOffset = mod.active
                ? mod.x_offset + mod.x // keep the same counter
                : songIsPlaying
                ? songTimeInSeconds
                : (float)internalClock;
        }

        rnd.trigger(voiceId + 1, phaseOffset);
        if (resetGlobalVoice && !mod.active) {
            rnd.trigger(0, phaseOffset);
        }
    }
}

std::vector<Modulation::Connection> Modulation::getConnections()
{
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<Connection> conns;
    conns.reserve(connections.size());

    for (auto& ptr : connections)
        conns.push_back(*ptr);

    return conns;
}

/*
* Generates a random 0..1 from a note timestamp
* The random seed is XOR'ed with the start time stamp
* such that each plugin run sounds different
*/
float Modulation::randFromVoiceTimestamp(uint64_t ts)
{
    auto hash = std::hash<uint64_t>{}(ts ^ start_ts);
    return static_cast<float>(hash / static_cast<double>(UINT64_MAX));
}

ValueTree Modulation::serialize()
{
    std::lock_guard<std::mutex> lock(mtx);
    auto dir = ValueTree("MODULATIONS");

    for (const auto& conn : connections) {
        auto element = ValueTree("CONNECTION");
        element.setProperty("id", var(conn->id), nullptr);
        element.setProperty("src", var(conn->src), nullptr);
        element.setProperty("dst", var(conn->dst), nullptr);
        element.setProperty("bipolar", var(conn->bipolar), nullptr);
        element.setProperty("bypass", var(conn->bypass), nullptr);
        element.setProperty("mapped", var(conn->mapped), nullptr);
        element.setProperty("mpoints", var(conn->mpoints), nullptr);
        dir.appendChild(element, nullptr);
    }

    return dir;
}

void Modulation::unserialize(const ValueTree state)
{
    std::lock_guard<std::mutex> lock(mtx);

    std::vector<Modulation::Connection*> ptrs;
    ptrs.reserve(connections.size());
    for (auto& conn : connections)
        ptrs.push_back(conn.get());

    // disconnect all
    for (auto* connPtr : ptrs)
        _disconnect(connPtr->src, connPtr->dst);

    for (int i = 0; i < state.getNumChildren(); ++i) {
        ValueTree conn = state.getChild(i);
		const auto id = (int)conn.getProperty("id");
        const auto source = conn.getProperty("src").toString();
        const auto dest = conn.getProperty("dst").toString();
		const auto bipolar = (bool)conn.getProperty("bipolar");
		const auto bypass = (bool)conn.getProperty("bupass");
		const auto mapped = (bool)conn.getProperty("mapped");
        const auto mpoints = conn.getProperty("mpoints", "").toString().toStdString();

        if (!source.isEmpty() && !dest.isEmpty()) {
            _connect(source.toStdString(), dest.toStdString(), id);
            auto c = getConnection(source.toStdString(), dest.toStdString());
            c->bipolar = bipolar;
            c->bypass = bypass;
            c->mapped = mapped;
            if (!mpoints.empty())
                c->mpoints = mpoints;
            stringToMap(c->mpoints, curvemaps[id]);
        }
    }
}

std::string Modulation::mapToString(Pattern& map)
{
    return map.serialize();
}

void Modulation::stringToMap(std::string points, Pattern& map)
{
    map.unserialize(points);
}
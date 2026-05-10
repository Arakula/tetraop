#include "Modulation.h"
#include "../PluginProcessor.h"

Modulation::Modulation(TetraOPAudioProcessor& p)
    : audioProcessor(p)
{
    connections.reserve(globals::MAX_MODULATIONS);
    start_ts = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

    velCurve.insertPoint(0.f, 0.f, -0.225f, 1);
    velCurve.insertPoint(1.f, 1.f, 0.f, 1);
    velCurve.buildSegments();

    for (int i = 1; i <= globals::MAX_MODULATIONS; ++i) {
        auto prefix = juce::String("mod") + juce::String(i) + "_";
        connectionAmountParams[i] = getParamHandle(prefix + "amt");
        connectionTensionParams[i] = getParamHandle(prefix + "ten");
    }

    for (int i = 0; i < globals::MAX_MACROS; ++i)
        macroParams[i] = getParamHandle("macro" + juce::String(i + 1));

    for (int i = 0; i < globals::MAX_ENVELOPES; ++i) {
        auto prefix = "env" + juce::String(i + 1);
        auto& handles = envParamHandles[i];
        handles.delay = getParamHandle(prefix + "_del");
        handles.attack = getParamHandle(prefix + "_att");
        handles.hold = getParamHandle(prefix + "_hld");
        handles.decay = getParamHandle(prefix + "_dec");
        handles.sustain = getParamHandle(prefix + "_sus");
        handles.release = getParamHandle(prefix + "_rel");
        handles.tensionAttack = getParamHandle(prefix + "_tenatt");
        handles.tensionDecay = getParamHandle(prefix + "_tendec");
        handles.tensionRelease = getParamHandle(prefix + "_tenrel");
    }

    for (int i = 0; i < globals::MAX_LFOS; ++i) {
        auto prefix = juce::String("lfo") + juce::String(i + 1);
        auto& handles = lfoParamHandles[i];
        handles.mode = getParamHandle(prefix + "_mode");
        handles.sync = getParamHandle(prefix + "_sync");
        handles.rate = getParamHandle(prefix + "_rate");
        handles.rateSync = getParamHandle(prefix + "_rate_sync");
        handles.delay = getParamHandle(prefix + "_delay");
        handles.delaySync = getParamHandle(prefix + "_delay_sync");
        handles.rise = getParamHandle(prefix + "_rise");
        handles.riseSync = getParamHandle(prefix + "_rise_sync");
        handles.smooth = getParamHandle(prefix + "_smooth");
    }

    for (int i = 0; i < globals::MAX_RNDS; ++i) {
        auto prefix = juce::String("rnd") + juce::String(i + 1);
        rndParamHandles[i].rate = getParamHandle(prefix + "_rate");
        rndParamHandles[i].rateSync = getParamHandle(prefix + "_rate_sync");
    }
}

void Modulation::parameterChanged(const juce::String& paramId, float value)
{
    auto& param = params[paramId];
    param.value = value;
    param.norm = param.range.convertTo0to1(value);
    smoothedParams.insert(paramId);
}

void Modulation::prepare()
{
    for (auto& [name, param] : params)
    {
        param.smoother.setup(globals::PARAM_SMOOTHER_RESISTANCE, audioProcessor.srate);
    }
}

static float getRateBeats(int sync, LFO::SyncMode mode)
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

    if (mode == LFO::Tripplet) {
        qn *= 2 / 3.f;
    }
    else if (mode == LFO::Dotted) {
        qn *= 1.5f;
    }

    return qn;
}

static float getDelayBeats(int sync, LFO::SyncMode mode)
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

    if (mode == LFO::Tripplet) {
        qn *= 2 / 3.f;
    }
    else if (mode == LFO::Dotted) {
        qn *= 1.5f;
    }

    return qn;
}

// update connection values
void Modulation::tickConnections(int blkoffset, float srate)
{
    for (auto& conn : connections) {
        auto amount = getValue(connectionAmountParams[conn->id], false, blkoffset, srate);
        auto tension = getValue(connectionTensionParams[conn->id], false, blkoffset, srate);
        conn->amount = amount;
        if (conn->tension != tension) {
            conn->tension = tension;
            conn->power = std::pow(1.1f, std::fabs(tension * globals::POWER_CURVE_POWER));
        }
    }
}

// macros are both a modulator and destination
void Modulation::tickMacros(int blkoffset, float srate)
{
    auto lastVoice = (Voice*)audioProcessor.synth->getVoice(lastUsedVoice);
    for (int i = 0; i < globals::MAX_MACROS; ++i) {
        auto id = "macro" + juce::String(i + 1);
        auto& mod = modulators[id];
        if (mod.connections == 0) continue;
        mod.active = lastVoice->isActive() || lastVoice->pressed;
        mod.value = getValue(macroParams[i], false, blkoffset, srate);
    }
}

void Modulation::tick(double srate, int nsamples, float secondsPerBeat)
{
    std::lock_guard<std::mutex> lock(mtx);

    // FIX standalone build
    if (secondsPerBeat == 0.f)
        secondsPerBeat = 0.25f;

    // update connections before modulators that depend on them
    tickConnections(nsamples, (float)srate);

    // init envelopes
    for (auto i = 0; i < globals::MAX_ENVELOPES; ++i) {
        auto prefix = "env" + juce::String(i + 1);
        auto& mod = modulators[prefix];
        if (i > 1 && mod.connections == 0) continue;
        auto mode = (Envelope::Mode)audioProcessor.params.getRawParameterValue(prefix + "_mode")->load();
        auto& handles = envParamHandles[i];
        float delay = getValue(handles.delay);
        float attack = getValue(handles.attack);
        float hold = getValue(handles.hold);
        float decay = getValue(handles.decay);
        float sustain = getValue(handles.sustain);
        float release = getValue(handles.release);
        float tenatt = getValue(handles.tensionAttack);
        float tendec = getValue(handles.tensionDecay);
        float tenrel = getValue(handles.tensionRelease);
        auto& env = envs[i];
        env.init(mode, delay, attack, hold, decay, sustain, release, tenatt, tendec, tenrel);
    }

    // init lfos
    for (auto i = 0; i < globals::MAX_LFOS; ++i) {
        auto prefix = juce::String("lfo") + juce::String(i + 1);
        auto& mod = modulators[prefix];
        if (mod.connections == 0) continue;
        auto& lfo = lfos[i];
        auto& handles = lfoParamHandles[i];
        auto mode = (LFO::Mode)getValue(handles.mode, true);
        auto sync = (LFO::SyncMode)getValue(handles.sync, true);

        auto rate = sync == LFO::SyncMode::Rate
            ? 1.f / getValue(handles.rate)
            : getRateBeats((int)getValue(handles.rateSync), sync);
        if (sync != LFO::Rate)
            rate *= secondsPerBeat;

        auto delay = sync == LFO::SyncMode::Rate
            ? getValue(handles.delay)
            : getDelayBeats((LFO::SyncMode)getValue(handles.delaySync), sync);
        if (sync != LFO::Rate)
            delay *= secondsPerBeat;

        auto rise = sync == LFO::SyncMode::Rate
            ? getValue(handles.rise)
            : getDelayBeats((LFO::SyncMode)getValue(handles.riseSync), sync);
        if (sync != LFO::Rate)
            rise *= secondsPerBeat;

        auto smooth = getValue(handles.smooth);

        auto duration = rate;
        bool srateChanged = lfo.srate != srate;
        lfo.init(srate, duration, delay, rise, mode);
        if (lfo.smooth != smooth || srateChanged) {
            lfo.setSmooth(smooth * smooth * 0.25f);
        }
    }

    // init rand generators
    for (int i = 0; i < globals::MAX_RNDS; ++i) {
        auto prefix = juce::String("rnd") + juce::String(i + 1);
        auto& mod = modulators[prefix];
        if (mod.connections == 0) continue;
        auto mode = (RandGen::Mode)audioProcessor.params.getRawParameterValue(prefix + "_mode")->load();
        auto sync = (LFO::SyncMode)audioProcessor.params.getRawParameterValue(prefix + "_sync")->load();
        auto global = (LFO::SyncMode)audioProcessor.params.getRawParameterValue(prefix + "_global")->load();

        auto rate = sync == LFO::SyncMode::Rate
            ? getValue(rndParamHandles[i].rate)
            : getRateBeats((int)getValue(rndParamHandles[i].rateSync), sync);
        if (sync != LFO::Rate)
            rate *= secondsPerBeat;

        auto& rnd = rnds[i];
        rnd.init(rate, mode, global);
    }

    auto lastVoice = (Voice*)audioProcessor.synth->getVoice(lastUsedVoice);
    lastVoiceActive = lastVoice->isActive() || lastVoice->pressed;

    blockDelta = (float)(nsamples / srate);
    float dt = blockDelta;
    internalClock += (double)dt;

    // tick envelopes
    for (int i = 0; i < globals::MAX_ENVELOPES; ++i) {
        auto& mod = modulators["env" + juce::String(i + 1)];
        if (i > 1 && mod.connections == 0) { // first two envelopes are connected to l1 noise/sub and l2 noise/sub
            mod.active = false;
            continue;
        }
        auto& env = envs[i];
        float value = !lastVoice->pressed && !lastVoice->released
            ? env.getValue(0.f, 0.f)
            : env.getValue(lastVoice->attack_elapsed, lastVoice->release_elapsed, dt, lastVoice->released);

        // store modulator values
        mod.value = value;
        mod.released = lastVoice->released;
        mod.x = env.mode == Envelope::AHD
            ? lastVoice->attack_elapsed + lastVoice->release_elapsed + dt
            : lastVoice->released
                ? lastVoice->release_elapsed + dt
                : lastVoice->attack_elapsed + dt;
        mod.release_y = env.lrelgain;
        mod.active = lastVoiceActive || (mod.value > 0.f &&
            (lastVoice->release_elapsed != 0.0f || lastVoice->attack_elapsed != 0.0f));
    }

    // tick LFOS
    for (int i = 0; i < globals::MAX_LFOS; ++i) {
        auto& mod = modulators["lfo" + juce::String(i + 1)];
        if (mod.connections == 0) {
            mod.active = false;
            continue;
        }
        auto& lfo = lfos[i];
        auto elapsed = !lastVoice->pressed && !lastVoice->released
            ? 0.f // initial voice state
            : lfo.mode == LFO::Trigger || lfo.mode == LFO::Envelope
            ? lastVoice->attack_elapsed + lastVoice->release_elapsed
            : lfo.voices[0].x; // Global mode

        float value = lfo.getSmoothedValue(elapsed, dt, 0, "dummy");
        mod.value = value;
        mod.x = elapsed + dt;
        mod.x_offset = lfo.voices[0].phase_offset;
        mod.active = lastVoiceActive;
    }

    // tick randgens
    for (int i = 0; i < globals::MAX_RNDS; ++i) {
        auto& mod = modulators["rnd" + juce::String(i + 1)];
        if (mod.connections == 0) {
            mod.active = false;
            continue;
        }
        auto& rnd = rnds[i];
        auto elapsed = !lastVoice->pressed && !lastVoice->released
            ? 0.f
            : rnd.globalSync ? mod.x
            : lastVoice->attack_elapsed + lastVoice->release_elapsed;

        float value = rnd.getValue(elapsed + dt, rnd.globalSync ? 0 : lastUsedVoice + 1);
        mod.value = value;
        mod.x = elapsed + dt;
        mod.x_offset = rnd.voices[0].phase_offset;
        mod.active = lastVoiceActive;
    }

    // Tick other modulators

    // update velocity modulator
    modulators["vel"].active = lastVoiceActive;
    modulators["vel"].x = lastVoice->vel;
    modulators["vel"].value = lastVoice->vel;

    // update keytrack modulator
    modulators["key"].active = lastVoiceActive;
    modulators["key"].x = lastVoice->key;
    modulators["key"].value = lastVoice->key;

    // update modwheel modulator
    modulators["mod"].active = lastVoiceActive;
    modulators["mod"].value = modwheelValue;

    // update pitchbend modulator
    modulators["bend"].active = lastVoiceActive;
    modulators["bend"].value = pitchbendValue;

    // expression pedal
    modulators["exp"].active = lastVoiceActive;
    modulators["exp"].value = expressPedalValue;

    // volume pedal
    modulators["vol"].active = lastVoiceActive;
    modulators["vol"].value = volumePedalValue;

    // sustain pedal
    modulators["sus"].active = lastVoiceActive;
    modulators["sus"].value = sustainPedalValue;

    // sustain pedal
    modulators["soft"].active = lastVoiceActive;
    modulators["soft"].value = softPedalValue;

    // update rand modulator
    modulators["rand"].active = lastVoiceActive;
    modulators["rand"].value = randFromVoiceTimestamp(lastVoice->pressed_ts);

    // update aftertouch mod
    modulators["at"].active = lastVoiceActive;
    modulators["at"].value = aftertouch.voices[lastVoice->id];

    // update MPE mods
    bool mpeActive = lastVoiceActive && lastVoice->mpe_channel > 1;
    modulators["x"].active = mpeActive;
    modulators["x"].value = lastVoice->mpe_channel > 1 ? mpe[lastVoice->mpe_channel].x : 0.f;
    modulators["y"].active = mpeActive;
    modulators["y"].value = lastVoice->mpe_channel > 1 ? mpe[lastVoice->mpe_channel].y : 0.f;
    modulators["z"].active = mpeActive;
    modulators["z"].value = lastVoice->mpe_channel > 1 ? mpe[lastVoice->mpe_channel].z : 0.f;
    modulators["lift"].active = mpeActive;
    modulators["lift"].value = (lastVoice->mpe_channel > 1 && lastVoice->released)
        ? mpe[lastVoice->mpe_channel].lift : 0.f;

    tickMacros(nsamples, (float)srate);
}

void Modulation::endBlock(int nsamples)
{
    auto dt = nsamples / audioProcessor.srate;
    for (auto it = smoothedParams.begin(); it != smoothedParams.end(); )
    {
        auto& param = params[*it];
        param.smoothedNorm = param.smoother.process(param.norm, dt);
        param.smoothedValue = param.range.convertFrom0to1(param.smoothedNorm);
        if (param.norm == param.smoothedNorm)
        {
            it = smoothedParams.erase(it);
        }
        else
        {
            ++it;
        }
    }

    timeElapsedSinceUIupdate += dt;
    if (timeElapsedSinceUIupdate > 0.016f) // ~aprox 60hz
    {
        timeElapsedSinceUIupdate = 0.f;
        // cache each modulated param value, used for UI display
        for (auto& [dst, conns] : destinations) {
            float offset = calculateOffset(conns);
            auto param = audioProcessor.params.getParameter(dst);

            if ( param )
            {
                auto norm = param->getValue ();
                auto& p = getParam ( dst );
                p.modulatedNorm.store ( std::clamp ( norm + offset, 0.f, 1.f ) );
            }
        }
    }
}

/*
* Public connect method, locks behind a mutex
*/
void Modulation::connect(const juce::String& src, const juce::String& dst, int sliderId)
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
void Modulation::_connect(const juce::String& src, const juce::String& dst, int sliderId, int connIndex)
{
    if (connections.size() == globals::MAX_MODULATIONS) {
        //Z_ERR ("_connect: max modulations reached (" << globals::MAX_MODULATIONS << ")");
        return;
    }

    std::unordered_set<int> usedSliderIds;
    for (auto& conn : connections) {
        usedSliderIds.insert(conn->id);
    }

    if (sliderId > 0 && usedSliderIds.count(sliderId) > 0)
        return;

    if (sliderId == 0) {
        // pick an unused slider
        for (int id = 1; id <= globals::MAX_MODULATIONS; ++id) {
            if (usedSliderIds.find(id) == usedSliderIds.end()) {
                sliderId = id;
                break;
            }
        }

        // reset slider to default values
        auto amt = audioProcessor.params.getParameter(juce::String("mod") + juce::String(sliderId) + "_amt");
        amt->setValueNotifyingHost(amt->getDefaultValue());
        auto ten = audioProcessor.params.getParameter(juce::String("mod") + juce::String(sliderId) + "_ten");
        ten->setValueNotifyingHost(ten->getDefaultValue());
    }

    juce::String ssrc = juce::String(src);
    bool bipolar = ssrc.startsWith("lfo") || ssrc == "key" || ssrc == "x" || ssrc == "bend";

    auto conn = std::make_unique<Connection>(Connection{ src, dst, bipolar, sliderId, false });
    if (ssrc.startsWith("env")) {
        auto index = ssrc.retainCharacters("0123456789").getIntValue() - 1;
        if (index >= 0 && index < globals::MAX_ENVELOPES) {
            conn->sourceKind = Connection::SourceKind::Env;
            conn->sourceIndex = index;
        }
    }
    else if (ssrc.startsWith("lfo")) {
        auto index = ssrc.retainCharacters("0123456789").getIntValue() - 1;
        if (index >= 0 && index < globals::MAX_LFOS) {
            conn->sourceKind = Connection::SourceKind::Lfo;
            conn->sourceIndex = index;
        }
    }
    else if (ssrc.startsWith("rnd")) {
        auto index = ssrc.retainCharacters("0123456789").getIntValue() - 1;
        if (index >= 0 && index < globals::MAX_RNDS) {
            conn->sourceKind = Connection::SourceKind::Rnd;
            conn->sourceIndex = index;
        }
    }
    else if (ssrc == "mod") conn->sourceKind = Connection::SourceKind::Mod;
    else if (ssrc == "bend") conn->sourceKind = Connection::SourceKind::Bend;
    else if (ssrc == "key") conn->sourceKind = Connection::SourceKind::Key;
    else if (ssrc == "vel") conn->sourceKind = Connection::SourceKind::Vel;
    else if (ssrc == "rand") conn->sourceKind = Connection::SourceKind::Rand;
    else if (ssrc.startsWith("macro")) {
        conn->sourceKind = Connection::SourceKind::Macro;
        conn->sourceParam = getParamHandle(src);
    }
    else if (ssrc == "at") conn->sourceKind = Connection::SourceKind::Aftertouch;
    else if (ssrc == "x") conn->sourceKind = Connection::SourceKind::MpeX;
    else if (ssrc == "y") conn->sourceKind = Connection::SourceKind::MpeY;
    else if (ssrc == "z") conn->sourceKind = Connection::SourceKind::MpeZ;
    else if (ssrc == "lift") conn->sourceKind = Connection::SourceKind::MpeLift;

    conn->skipKeyTrackOffset = conn->sourceKind == Connection::SourceKind::Key
        && (dst.endsWith("pitch") || dst.endsWith("pitch_semis") || (dst.startsWith("f") && dst.endsWith("_cut")));

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
    auto& p = getParam(dst);
    ptr->dstParam = &p;
    p.connections += 1;
    p.dstConnections = &destinations[dst];
    modulators[src].connections += 1;
    UIDirty.store(true);
}

/*
* Public disconnect method with mutex lock
*/
void Modulation::disconnect(const juce::String& src, const juce::String& dst)
{
    std::lock_guard<std::mutex> lock(mtx);
    _disconnect(src, dst);
}

/*
* Inner disconect method, no mutex lock
*/
void Modulation::_disconnect(const juce::String& src, const juce::String& dst)
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

        auto& p = getParam(dst);
        p.connections -= 1;
        p.dstConnections = dstList.empty() ? nullptr : &dstList;
        modulators[src].connections -= 1;
        connections.erase(it);
        UIDirty.store(true);
    }
}

void Modulation::disconnectSelectedMod(const juce::String& dst)
{
    if (selectedMod.isEmpty()) return;
    disconnect(selectedMod, dst);
}

bool Modulation::isConnected(const juce::String& param)
{
    auto it = params.find(param);
    return it != params.end() && it->second.connections > 0;
}

bool Modulation::isSrcConnected(const juce::String& src)
{
    auto it = sources.find(src);
    return it != sources.end() && !it->second.empty();
}

Modulation::Connection* Modulation::getConnection(const juce::String& src, const juce::String& dst)
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
void Modulation::changeConnection(const juce::String& src, const juce::String& dst, const juce::String newsrc, const juce::String newdst)
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

void Modulation::setConnectionBypass(const juce::String& src, const juce::String& dst, bool bypass)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto* conn = getConnection(src, dst);
    if (conn != nullptr) {
        conn->bypass = bypass;
        UIDirty.store(true);
    }
}

void Modulation::setConnectionBipolar(const juce::String& src, const juce::String& dst, bool bipolar)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto* conn = getConnection(src, dst);
    if (conn != nullptr) {
        conn->bipolar = bipolar;
        UIDirty.store(true);
    }
}

void Modulation::setConnectionMPoints(const juce::String& src, const juce::String& dst, juce::String& mpoints)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto* conn = getConnection(src, dst);
    if (conn != nullptr) {
        conn->mpoints = mpoints.toStdString();
        stringToMap(mpoints.toStdString(), curvemaps[conn->id]);
        UIDirty.store(true);
    }
}

void Modulation::setConnectionMapped(const juce::String& src, const juce::String& dst, bool mapped)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto* conn = getConnection(src, dst);
    if (conn != nullptr) {
        conn->mapped = mapped;
        UIDirty.store(true);
    }
}

/**
 * juce::Thread unsafe get modulated value
 */
float Modulation::getValue(const juce::String& pname, bool useCache, int blockOffset, float srate, bool smooth)
{
    return getValue(&getParam(pname), useCache, blockOffset, srate, smooth);
}

float Modulation::getValue(const Param* param, bool useCache, int blockOffset, float srate, bool smooth)
{
    jassert(param != nullptr);
    if (useCache) {
        return param->value;
    }
    else {
        if (param->dstConnections != nullptr && !param->dstConnections->empty())
        {
            float offset = calculateOffset(*param->dstConnections, lastUsedVoice, blockOffset, srate);
            auto norm = smooth ? param->smoothedNorm : param->norm;
            return param->range.convertFrom0to1(std::clamp(norm + offset, 0.f, 1.f));
        }
        return smooth
            ? param->smoothedValue
            : param->value;
    }
}

/**
 * juce::Thread safe get modulated norm
 */
float Modulation::getModulatedNormSafe(const juce::String& param)
{
    return getParam(param).modulatedNorm.load();
}

/**
 * Retrieves an internally tracked param
 * If the param is not registered yet registers it and starts listening to events
 */
Modulation::Param& Modulation::getParam(const juce::String& pname)
{
    auto it = params.find(pname);
    if (it == params.end())
    {
        auto* param = audioProcessor.params.getParameter ( pname );

        auto& entry = params [ pname ]; // inserts and returns reference
        entry.id = pname;

        if ( param == nullptr )
        {
            // we still return the (new and initialized) entry for now, but unknown or old parameters should be handled earlier during preset loading
            //Z_ERR ( "Parameter not found: " << pname );
            return entry;
        }

        entry.norm = param->getValue();
        entry.value = audioProcessor.params.getRawParameterValue(pname)->load();
        entry.range = param->getNormalisableRange();
        auto dstIt = destinations.find(pname);
        entry.dstConnections = dstIt != destinations.end() ? &dstIt->second : nullptr;
        entry.smoother.setup(globals::PARAM_SMOOTHER_RESISTANCE, audioProcessor.srate);
        entry.smoother.reset(entry.norm);
        entry.smoothedNorm = entry.norm;
        entry.smoothedValue = entry.value;

        audioProcessor.params.addParameterListener(pname, this);
        return entry;
    }
    else
    {
        return it->second;
    }
}

Modulation::Param* Modulation::getParamHandle(const juce::String& pname)
{
    return &getParam(pname);
}

/**
 * Recalculate value for a specific voice
 */
float Modulation::getPolyValue(const juce::String& pname, int voiceId, int blockOffset, bool smooth)
{
    return getPolyValue(getParamHandle(pname), voiceId, blockOffset, smooth);
}

float Modulation::getPolyValue(const Param* param, int voiceId, int blockOffset, bool smooth)
{
    jassert(param != nullptr);
    if (param->dstConnections != nullptr && !param->dstConnections->empty())
    {
        float offset = calculateOffset(*param->dstConnections, voiceId, blockOffset);
        auto norm = smooth ? param->smoothedNorm : param->norm;
        norm = std::clamp(norm + offset, 0.f, 1.f);
        return param->range.convertFrom0to1(norm);
    }
    return smooth
        ? param->smoothedValue
        : param->value;
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
float Modulation::calculateOffset(std::vector<Connection*> conns, int voiceId, int blockOffset, float srate)
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
            if (conn->bipolar) value *= 0.5f;

            return value;
        };

    float offset = 0.f;
    auto voice = voiceId == -1
        ? nullptr
        : (Voice*)audioProcessor.synth->getVoice(voiceId);

    auto dt = blockOffset * (srate > 0.f ? 1.f / srate : audioProcessor.iosrate);

    for (auto* conn : conns) {
        if (conn->bypass) continue;
        float srcValue = 0.0f;
        auto& mod = modulators[conn->src];

        if (voice == nullptr) { // no voice, global modulator, use cached value
            srcValue = mod.value;
            if (!mod.active)
                continue; // FIX
        }
        else if (conn->sourceKind == Connection::SourceKind::Env) {
            auto& env = envs[conn->sourceIndex];
            srcValue = env.getValue(
                voice->attack_elapsed,
                voice->release_elapsed,
                dt,
                voice->released
            );
        }
        else if (conn->sourceKind == Connection::SourceKind::Lfo) {
            auto& lfo = lfos[conn->sourceIndex];
            auto elapsed = voice->attack_elapsed + voice->release_elapsed;
            srcValue = lfo.getSmoothedValue(elapsed, dt, voiceId + 1, conn->dst);
        }
        else if (conn->sourceKind == Connection::SourceKind::Rnd) {
            auto& rnd = rnds[conn->sourceIndex];
            auto elapsed = rnd.globalSync
                ? mod.x + dt
                : voice->attack_elapsed + voice->release_elapsed + dt;
            srcValue = rnd.getValue(elapsed, rnd.globalSync ? 0 : voiceId + 1);
        }
        else if (conn->sourceKind == Connection::SourceKind::Mod) {
            srcValue = modwheelValue;
        }
        else if (conn->sourceKind == Connection::SourceKind::Bend) {
            srcValue = pitchbendValue;
        }
        else if (conn->sourceKind == Connection::SourceKind::Key) {
            // keytracking for pitch and filter cut is implemented in getKeyTrackFor()
            if (conn->skipKeyTrackOffset) {
                continue; // skip this calc completely
            }
            srcValue = voice->key;
        }
        else if (conn->sourceKind == Connection::SourceKind::Vel) {
            srcValue = voice->vel;
        }
        else if (conn->sourceKind == Connection::SourceKind::Rand) {
            srcValue = randFromVoiceTimestamp(voice->pressed_ts);
        }
        else if (conn->sourceKind == Connection::SourceKind::Macro) {
            srcValue = getValue(conn->sourceParam, true);
        }
        else if (conn->sourceKind == Connection::SourceKind::Aftertouch) {
            if (audioProcessor.mpe_enabled)
                continue;
            srcValue = aftertouch.voices[voice->id];
        }
        else if (conn->sourceKind == Connection::SourceKind::MpeX) {
            if (voice->mpe_channel <= 1 || !audioProcessor.mpe_enabled)
                continue;
            srcValue = mpe[voice->mpe_channel].x;
        }
        else if (conn->sourceKind == Connection::SourceKind::MpeY) {
            if (voice->mpe_channel <= 1 || !audioProcessor.mpe_enabled)
                continue;
            srcValue = mpe[voice->mpe_channel].y;
        }
        else if (conn->sourceKind == Connection::SourceKind::MpeZ) {
            if (voice->mpe_channel <= 1 || !audioProcessor.mpe_enabled)
                continue;
            srcValue = mpe[voice->mpe_channel].z;
        }
        else if (conn->sourceKind == Connection::SourceKind::MpeLift) {
            if (voice->mpe_channel <= 1 || !audioProcessor.mpe_enabled || !voice->released)
                continue;
            srcValue = mpe[voice->mpe_channel].lift;
        }
        else if (conn->sourceKind == Connection::SourceKind::Other) {
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
    for (int i = 0; i < globals::MAX_POLYPHONY; ++i) {
        aftertouch.voices[i] = value;
    }
}

void Modulation::setSelectedMod(const juce::String& id)
{
    selectedMod = id;
    if (id.startsWith("env")) {
        audioProcessor.displayEnv = id;
    }
    if (id.startsWith("lfo")) {
        audioProcessor.displayLfo = id;
    }
    if (id.startsWith("rnd")) {
        // TODO
    }
    UIDirty.store(true);
}

bool Modulation::isAnyVoiceActive()
{
    auto lastVoice = (Voice*)audioProcessor.synth->getVoice(lastUsedVoice);
    return lastVoice->isActive() || lastVoice->pressed;
}

float Modulation::getKeyTrackFor(const juce::String& param)
{
    std::lock_guard<std::mutex> lock(mtx);
    auto* conn = getConnection("key", param);
    if (conn == nullptr || conn->bypass)
        return 0.f;

    return conn->amount;
}

float Modulation::getKeyTrackOffsetFor(const juce::String& param, int note)
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
    for (int i = 0; i < globals::MAX_LFOS; ++i) {
        auto& mod = modulators["lfo" + juce::String(i + 1)];
        if (mod.connections == 0) continue;
        auto& lfo = lfos[i];

        auto phaseOffset = 0.f;
        auto globalValue = 0.f; // for sync LFOs restart them at last global lfo value (fixes global smooth lfos)
        if (lfo.mode == LFO::Mode::Sync) {
            phaseOffset = mod.active
                ? mod.x_offset + mod.x + lfo.delay // keep the voice LFOs in sync with global lfo
                : songIsPlaying
                ? songTimeInSeconds + lfo.delay // make the first voice running this LFO in sync with song position
                : (float)internalClock; // if the song is not playing use a global timer to start the LFO somewhere
            globalValue = lfo.voices[0].y;

            // HACK
            // force all connections of this LFO to create LFO smoothers
            // so that on lfo.trigger() the smoothers already exist
            // this whole LFO + Sync + Smooth logic should be simplified
            auto& srcs = sources["lfo" + juce::String(i + 1)];
            for (auto& conn : srcs) {
                getValue(conn->dstParam, false, 0);
            }
        }

        lfo.trigger(voiceId + 1, phaseOffset, globalValue);
        if (resetGlobalVoice) {
            lfo.trigger(0, phaseOffset, globalValue);
        }
    }

    for (int i = 0; i < globals::MAX_RNDS; ++i) {
        auto& mod = modulators["rnd" + juce::String(i + 1)];
        if (mod.connections == 0) continue;
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

juce::ValueTree Modulation::serialize()
{
    std::lock_guard<std::mutex> lock(mtx);
    auto dir = juce::ValueTree("MODULATIONS");

    for (const auto& conn : connections) {
        auto element = juce::ValueTree("CONNECTION");
        element.setProperty("id", juce::var(conn->id), nullptr);
        element.setProperty("src", juce::var(conn->src), nullptr);
        element.setProperty("dst", juce::var(conn->dst), nullptr);
        element.setProperty("bipolar", juce::var(conn->bipolar), nullptr);
        element.setProperty("bypass", juce::var(conn->bypass), nullptr);
        element.setProperty("mapped", juce::var(conn->mapped), nullptr);
        element.setProperty("mpoints", juce::var(conn->mpoints), nullptr);
        dir.appendChild(element, nullptr);
    }

    return dir;
}

void Modulation::unserialize(const juce::ValueTree state)
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
        juce::ValueTree conn = state.getChild(i);
		const auto id = (int)conn.getProperty("id");
        const auto source = conn.getProperty("src").toString();
        const auto dest = conn.getProperty("dst").toString();
		const auto bipolar = (bool)conn.getProperty("bipolar");
		const auto bypass = (bool)conn.getProperty("bypass");
		const auto mapped = (bool)conn.getProperty("mapped");
        const auto mpoints = conn.getProperty("mpoints", "").toString();

        if (!source.isEmpty() && !dest.isEmpty()) {
            _connect(source, dest, id);
            auto c = getConnection(source, dest);
            c->bipolar = bipolar;
            c->bypass = bypass;
            c->mapped = mapped;
            if (!mpoints.isEmpty())
                c->mpoints = mpoints;
            stringToMap(c->mpoints, curvemaps[id]);
        }
    }
}

juce::String Modulation::mapToString(Pattern& map)
{
    return map.serialize();
}

void Modulation::stringToMap(juce::String points, Pattern& map)
{
    map.unserialize(points.toStdString());
}

bool Modulation::isFmMatrixModulated()
{
    for (auto& conn : connections)
    {
        auto& dst = conn->dst;
        if (dst.startsWith("fm_") || dst.startsWith("rm_") || dst.endsWith("feedback"))
            return true;
    }
    return false;
}
#include "../../PluginEditor.h"
#include "EnvDisplay.h"

EnvDisplay::EnvDisplay(TetraOPAudioProcessorEditor& e)
    : editor(e)
{
    startTimerHz(60);

    delay = std::make_unique<Rotary>(editor, "env1_del", "Del", Rotary::secondsMillis);
    attack = std::make_unique<Rotary>(editor, "env1_att", "Att", Rotary::secondsSubMillis);
    hold = std::make_unique<Rotary>(editor, "env1_hld", "Hld", Rotary::secondsMillis);
    decay = std::make_unique<Rotary>(editor, "env1_dec", "Dec", Rotary::secondsMillis);
    sustain = std::make_unique<Rotary>(editor, "env1_sus", "Sus", Rotary::Percent);
    release = std::make_unique<Rotary>(editor, "env1_rel", "Rel", Rotary::secondsMillis);

    for (int i = 0; i < globals::MAX_ENVELOPES; ++i) {
        auto env = std::make_unique<Modulator>(editor, juce::String("env") + juce::String(i + 1));
        addAndMakeVisible(env.get());
        envs.push_back(std::move(env));
    }

    addAndMakeVisible(delay.get());
    addAndMakeVisible(attack.get());
    addAndMakeVisible(hold.get());
    addAndMakeVisible(decay.get());
    addAndMakeVisible(sustain.get());
    addAndMakeVisible(release.get());

    addAndMakeVisible(modeBtn);
    modeBtn.setAlpha(0.f);
    modeBtn.onClick = [this] { showEnvelopeModeMenu(); };
}

EnvDisplay::~EnvDisplay()
{
    editor.audioProcessor.params.removeParameterListener(envid + "_mode", this);
    editor.audioProcessor.params.removeParameterListener(envid + "_del", this);
    editor.audioProcessor.params.removeParameterListener(envid + "_att", this);
    editor.audioProcessor.params.removeParameterListener(envid + "_hld", this);
    editor.audioProcessor.params.removeParameterListener(envid + "_dec", this);
    editor.audioProcessor.params.removeParameterListener(envid + "_sus", this);
    editor.audioProcessor.params.removeParameterListener(envid + "_rel", this);
    editor.audioProcessor.params.removeParameterListener(envid + "_tenatt", this);
    editor.audioProcessor.params.removeParameterListener(envid + "_tendec", this);
    editor.audioProcessor.params.removeParameterListener(envid + "_tenrel", this);
}

void EnvDisplay::disconnect()
{
    if (envid.isNotEmpty()) {
        editor.audioProcessor.params.removeParameterListener(envid + "_mode", this);
        editor.audioProcessor.params.removeParameterListener(envid + "_del", this);
        editor.audioProcessor.params.removeParameterListener(envid + "_att", this);
        editor.audioProcessor.params.removeParameterListener(envid + "_hld", this);
        editor.audioProcessor.params.removeParameterListener(envid + "_dec", this);
        editor.audioProcessor.params.removeParameterListener(envid + "_sus", this);
        editor.audioProcessor.params.removeParameterListener(envid + "_rel", this);
        editor.audioProcessor.params.removeParameterListener(envid + "_tenatt", this);
        editor.audioProcessor.params.removeParameterListener(envid + "_tendec", this);
        editor.audioProcessor.params.removeParameterListener(envid + "_tenrel", this);
        editor.unregisterModParam(delay.get());
        editor.unregisterModParam(attack.get());
        editor.unregisterModParam(hold.get());
        editor.unregisterModParam(decay.get());
        editor.unregisterModParam(sustain.get());
        editor.unregisterModParam(release.get());
    }
    envid = "";
}

void EnvDisplay::timerCallback()
{
    auto selectedModId = juce::String(editor.audioProcessor.modulation->selectedMod);
    //auto& displayMod = editor.audioProcessor.displayMod;

    if (selectedModId != envid && selectedModId.startsWith("env"))
    {
        connect(selectedModId);
        repaint();
    }
    else if (envid.isNotEmpty()) {
        auto& mod = editor.audioProcessor.modulation->modulators[envid];
        if (mod.active && (mod.connections || envid == "env1")) {
            repaint();
        }
        else if (!mod.active && isActive) {
            repaint(); // repaint one last time to remove the seek
        }
    }
}

void EnvDisplay::connect(juce::String id)
{
    disconnect();
    envid = id;
    envidx = envid.retainCharacters("0123456789").getIntValue() - 1;
    editor.audioProcessor.params.addParameterListener(envid + "_mode", this);
    editor.audioProcessor.params.addParameterListener(envid + "_del", this);
    editor.audioProcessor.params.addParameterListener(envid + "_att", this);
    editor.audioProcessor.params.addParameterListener(envid + "_hld", this);
    editor.audioProcessor.params.addParameterListener(envid + "_dec", this);
    editor.audioProcessor.params.addParameterListener(envid + "_sus", this);
    editor.audioProcessor.params.addParameterListener(envid + "_rel", this);
    editor.audioProcessor.params.addParameterListener(envid + "_tenatt", this);
    editor.audioProcessor.params.addParameterListener(envid + "_tendec", this);
    editor.audioProcessor.params.addParameterListener(envid + "_tenrel", this);

    delay->setParamId(envid + "_del");
    attack->setParamId(envid + "_att");
    hold->setParamId(envid + "_hld");
    decay->setParamId(envid + "_dec");
    sustain->setParamId(envid + "_sus");
    release->setParamId(envid + "_rel");

    editor.registerModParam(delay.get());
    editor.registerModParam(attack.get());
    editor.registerModParam(hold.get());
    editor.registerModParam(decay.get());
    editor.registerModParam(sustain.get());
    editor.registerModParam(release.get());

    editor.audioProcessor.modulation->UIDirty.store(true); // refresh connections
    toggleUIComponents();
}

void EnvDisplay::parameterChanged(const juce::String& parameterID, float newValue)
{
    (void)newValue;

    if (parameterID.endsWith("_mode"))
        juce::MessageManager::callAsync([this] { toggleUIComponents(); });

    juce::MessageManager::callAsync([this]() { repaint();  });
}

int EnvDisplay::getSection(const juce::MouseEvent& e)
{
    for (int i = 0; i < int (envStages.size()); ++i) {
        if (envStages[i].contains(e.x, e.y)) {
            return i;
        }
    }
    return -1;
}

void EnvDisplay::mouseMove(const juce::MouseEvent& e)
{
    highlighted = getSection(e);
    repaint();
}

void EnvDisplay::mouseExit(const juce::MouseEvent& e)
{
    (void)e;
    highlighted = -1;
    repaint();
}

void EnvDisplay::mouseDown(const juce::MouseEvent& e)
{
    auto section = getSection(e);
    if (section != 1 && section != 3 && section != 4)
        return;

    //editor.audioProcessor.undomgr->createUndo();

    paramId = envid + (section == 1 ? "_tenatt" : section == 3 ? "_tendec" : "_tenrel");
    UIUtils::startUnboundedMouse(*this, e);
    mouse_down = true;
    mouse_down_shift = e.mods.isShiftDown();
    auto param = editor.audioProcessor.params.getParameter(paramId);
    auto cur_val = param->getValue();
    cur_normed_value = cur_val;
    last_mouse_position = e.getPosition();
    start_mouse_pos = juce::Desktop::getInstance().getMousePosition();
    param->beginChangeGesture();
}

void EnvDisplay::mouseUp(const juce::MouseEvent& e)
{
    if (!mouse_down) return;
    auto h = highlighted;
    mouse_down = false;
    if (UIUtils::stopUnboundedMouse(*this, e))
        juce::Desktop::getInstance().setMousePosition(start_mouse_pos);
    auto param = editor.audioProcessor.params.getParameter(paramId);
    param->endChangeGesture();
    highlighted = h; // FIX flickering
}

void EnvDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (!mouse_down) return;
    auto change = e.getPosition() - last_mouse_position;
    last_mouse_position = e.getPosition();
    auto speed = (e.mods.isCommandDown() ? 40.0f : 4.0f) * 100.f;
    auto slider_change = float(change.getX() - change.getY()) / speed;
    cur_normed_value += slider_change;
    auto param = editor.audioProcessor.params.getParameter(paramId);

    param->setValueNotifyingHost(cur_normed_value);
}

void EnvDisplay::mouseDoubleClick(const juce::MouseEvent& e) {
    auto section = getSection(e);
    if (section != 1 && section != 3 && section != 4)
        return;

    //editor.audioProcessor.undomgr->createUndo();
    paramId = envid + (section == 1 ? "_tenatt" : section == 3 ? "_tendec" : "_tenrel");

    auto param = editor.audioProcessor.params.getParameter(paramId);
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->getDefaultValue());
    param->endChangeGesture();
}

void EnvDisplay::paint(juce::Graphics& g)
{
    g.setColour(COLOR_PANEL().darker(0.6f));
    g.fillRect(getLocalBounds());
    g.setColour(COLOR_BEVEL());
    g.fillRect(viewportBounds);

    if (envid.isEmpty() || !isVisible()) return;
    isActive = editor.audioProcessor.modulation->modulators[envid].active;
    auto globalTime = editor.audioProcessor.modulation->globalTimeFactor;
    auto mode = (Envelope::Mode)editor.audioProcessor.params.getRawParameterValue(envid + "_mode")->load();
    auto bounds = viewportBounds.reduced(4).toFloat();
    auto del = editor.audioProcessor.params.getRawParameterValue(envid + "_del")->load() * globalTime;
    auto att = std::max(0.0001f, editor.audioProcessor.params.getRawParameterValue(envid + "_att")->load()) * globalTime;
    auto hld = editor.audioProcessor.params.getRawParameterValue(envid + "_hld")->load() * globalTime;
    auto dec = std::max(0.0001f, editor.audioProcessor.params.getRawParameterValue(envid + "_dec")->load()) * globalTime;
    auto sus = editor.audioProcessor.params.getRawParameterValue(envid + "_sus")->load();
    auto rel = editor.audioProcessor.params.getRawParameterValue(envid + "_rel")->load() * globalTime;
    auto tatt = editor.audioProcessor.params.getRawParameterValue(envid + "_tenatt")->load();
    auto tdec = editor.audioProcessor.params.getRawParameterValue(envid + "_tendec")->load();
    auto trel = editor.audioProcessor.params.getRawParameterValue(envid + "_tenrel")->load();

    if (mode == Envelope::ADSR) {
        del = 0.f;
        hld = 0.f;
    }
    else if (mode == Envelope::AHD) {
        del = 0.f;
        sus = 0.f;
        rel = 0.0001f;
    }
    else if (mode == Envelope::DADSR) {
        hld = 0.f;
    }

    auto total = del + att + hld + dec + rel;
    if (total == 0.f) return;

    // ============================================================================
    // bunch of complicated math to compress the ratios of envelope segments using power law
    // makes the envelope less squished or expanded and more even
    std::array<float, 5> durations = { del, att, hld, dec, rel };
    std::array<float, 6> t_real_bounds = {};
    t_real_bounds[0] = 0.0f;
    for (int i = 0; i < 5; ++i) {
        auto idx = i + 1;
        t_real_bounds[idx] = t_real_bounds[i] + durations[i] / total;
    }
    std::array<float, 5> weights{};
    float wSum = 0.0f;
    for (int i = 0; i < 5; ++i) {
        weights[i] = std::pow(durations[i] + 1e-6f, 0.4f);
        wSum += weights[i];
    }
    std::array<float, 6> t_vis_bounds = {};
    t_vis_bounds[0] = 0.0f;
    for (int i = 0; i < 5; ++i)
        t_vis_bounds[i + 1] = t_vis_bounds[i] + weights[i] / wSum;

    auto visualToReal = [t_vis_bounds, t_real_bounds](float t_vis)
        {
            t_vis = std::clamp(t_vis, 0.0f, 1.0f);

            // Find which segment t_vis is in
            int region = 4;
            for (int i = 0; i < 5; ++i) {
                auto idx = i + 1;
                if (t_vis < t_vis_bounds[idx]) { region = i; break; }
            }
            float local = (t_vis - t_vis_bounds[region]) /
                (t_vis_bounds[region + 1] - t_vis_bounds[region]);
            float t_real = t_real_bounds[region] +
                local * (t_real_bounds[region + 1] - t_real_bounds[region]);

            return std::clamp(t_real, 0.f, 1.f);
        };
    auto realToVisual = [t_real_bounds, t_vis_bounds](float t_real)
        {
            t_real = std::clamp(t_real, 0.0f, 1.0f);

            int region = 4;
            for (int i = 0; i < 5; ++i) {
                if (t_real <= t_real_bounds[i + 1]) { region = i; break; }
            }

            float denom = t_real_bounds[region + 1] - t_real_bounds[region];
            float local = denom > 0.0f ? (t_real - t_real_bounds[region]) / denom : 0.0f;

            float t_vis = t_vis_bounds[region] + local * (t_vis_bounds[region + 1] - t_vis_bounds[region]);
            return std::clamp(t_vis, 0.0f, 1.0f);
        };
    // ===================================================================================

    if (highlighted > -1) {
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.fillRect(envStages[highlighted]);
    }

    Pattern p{-1};

    p.insertPoint(0.f, 0.f, tatt, 1);
    p.insertPoint(del / total, 0.f, tatt, 1);
    p.insertPoint((del + att) / total, 1.f, tdec, 1);
    p.insertPoint((del + att + hld) / total, 1.f, tdec, 1);
    p.insertPoint((del + att + hld + dec) / total, sus, trel, 1);
    p.insertPoint(1.f, 0.f, 0.f, 1);
    p.buildSegments();


    // draw envelope
    juce::Path path;
    auto pixels = bounds.getWidth();
    for (int x = 0; x < pixels; ++x) {
        float t = (float)x / (pixels - 1);
        float t_real = visualToReal(t);
        auto px = bounds.getX() + x;
        auto py = bounds.getY() + bounds.getHeight() * (1- p.get_y_at(t_real));
        if (x == 0)
            path.startNewSubPath(px, py);
        else
            path.lineTo(px, py);
    }

    g.setColour(COLOR_ENVELOPE());
    g.strokePath(path, juce::PathStrokeType(1.4f));
    path.closeSubPath();
    path.lineTo(bounds.getBottomRight());
    path.lineTo(bounds.getBottomLeft());
    path.closeSubPath();

    juce::ColourGradient grad(
        COLOR_ENVELOPE().withMultipliedAlpha(0.25f),
        bounds.getTopLeft().toFloat(),
        COLOR_ENVELOPE().withAlpha(0.0f),
        bounds.getBottomLeft().toFloat(),
        false
    );
    g.saveState();
    g.setGradientFill(grad);
    g.reduceClipRegion(path);
    g.fillRect(bounds);
    g.restoreState();

    // draw seek
    juce::Path seek;
    auto& mod = editor.audioProcessor.modulation->modulators[envid];
    auto seekwidth = 30;
    if (mod.active && (mod.connections || envid == "env1")) {
        float t_real_end = mod.released && mode != Envelope::AHD
            ? (del + att + hld + dec + mod.x) / total
            : std::min((del + att + hld + dec) / total, mod.x / total);

        float t_vis_end = realToVisual(t_real_end);

        int xEnd = std::min<int>((int)(t_vis_end * pixels), (int)pixels - 1);
        int xStart = std::max<int>(0, xEnd - seekwidth);

        auto points = std::vector<juce::Point<float>>{};
        for (int x = xStart; x <= xEnd; ++x) {
            float t_vis = (float)x / (pixels - 1);
            float t_real = visualToReal(t_vis);
            
            auto px = bounds.getX() + x;
            auto py = bounds.getY() + bounds.getHeight() * (1.0f - p.get_y_at(t_real));
            points.push_back({ px, py });
        }

        if (!points.empty() && xEnd != pixels - 1) {
            juce::Colour c = COLOR_ENVELOPE().brighter(1.0f);
            for (int i = 0; i < int (points.size()) - 1; ++i)
            {
                g.setColour(c.withAlpha((i + 1) / (float)points.size()));
                auto& p1 = points[i];
                auto& p2 = points[i + 1];
                g.drawLine(p1.x, p1.y, p2.x, p2.y, 2.f);
            }
        }
    }

    // recompute envelope sections
    for (int i = 0; i < 5; ++i) {
        auto next = i + 1;
        float t_vis_start = t_vis_bounds[i];
        float t_vis_end = t_vis_bounds[next];

        int xStart = (int)(t_vis_start * pixels);
        int xEnd = (int)(t_vis_end * pixels);

        envStages[i] = { (int)bounds.getX() + xStart, (int)bounds.getY(), xEnd - xStart, (int)bounds.getHeight()};
    }

    // draw mode btn
    auto text = mode == 0 ? "ADSR" : mode == 1 ? "AHD" : mode == 2 ? "DADSR" : "DAHDSR";
    g.setFont(juce::FontOptions(11.f));
    g.setColour(COLOR_VIEWPORT_TEXT());
    g.drawText(text, modeBtn.getBounds(), juce::Justification::centredRight);
    //UIUtils::drawTriangle(g, modeBtn.getBounds().withWidth(15).withX(modeBtn.getRight() - 15)
    //    .toFloat().reduced(4.f), 2, COLOR_VIEWPORT_TEXT_DIM());
}

void EnvDisplay::resized()
{
    auto bounds = getLocalBounds().toFloat();
    viewportBounds = getLocalBounds()
        .withTrimmedTop(32 + 4)
        .withTrimmedBottom(globals::KNOB_HEIGHT - 10).reduced(2,5).toNearestInt();

    float gap = 2.f;
    auto modw = (bounds.getWidth() - gap * 5.f) / 4.f;
    auto modh = 37.f;
    for (int i = 0; i < globals::MAX_ENVELOPES; ++i) {
        auto& env = envs[i];
        env->setBounds((int)(bounds.getX() + i * gap + i * modw + gap), (int)(bounds.getY() + gap - 1.f), (int)modw, (int)modh);
    }

    auto& env = envs[MAX_ENVELOPES - 1];
    env->setBounds(env->getBounds().withRight(int(bounds.getRight() - (int)gap))); // visual fix

    toggleUIComponents();
}

void EnvDisplay::toggleUIComponents()
{
    delay->setVisible(false);
    attack->setVisible(false);
    hold->setVisible(false);
    decay->setVisible(false);
    sustain->setVisible(false);
    release->setVisible(false);

    auto bounds = getLocalBounds();
    modeBtn.setBounds(bounds.getRight() - 60 - 15, viewportBounds.getY() + 5, 60, 15);

    auto mode = envid.isNotEmpty()
        ? editor.audioProcessor.params.getRawParameterValue(envid + "_mode")->load()
        : 0;

    auto knobw = int(globals::KNOB_WIDTH);
    auto knobh = int(globals::KNOB_HEIGHT);
    int starty = bounds.getBottom() - globals::KNOB_HEIGHT;

    delay->radius = mode == 3 ? 11.f : 14.f;
    attack->radius = mode == 3 ? 11.f : 14.f;
    hold->radius = mode == 3 ? 11.f : 14.f;
    decay->radius = mode == 3 ? 11.f : 14.f;
    sustain->radius = mode == 3 ? 11.f : 14.f;
    release->radius = mode == 3 ? 11.f : 14.f;

    if (mode == 1) { // ADH
        int startX = int((getWidth() - 3 * knobw) / 2 - 2);
        attack->setVisible(true);
        hold->setVisible(true);
        decay->setVisible(true);
        attack->setBounds((int)getX() + startX, starty, knobw, knobh);
        hold->setBounds(attack->getRight(), starty, knobw, knobh);
        decay->setBounds(hold->getRight(), starty, knobw, knobh);
    }
    else if (mode == 2) { // DADSR
        int startX = int((getWidth() - 5 * knobw) / 2 - 2);
        delay->setVisible(true);
        attack->setVisible(true);
        decay->setVisible(true);
        sustain->setVisible(true);
        release->setVisible(true);

        delay->setBounds((int)getX() + startX, starty, knobw, knobh);
        attack->setBounds(delay->getRight(), starty, knobw, knobh);
        decay->setBounds(attack->getRight(), starty, knobw, knobh);
        sustain->setBounds(decay->getRight(), starty, knobw, knobh);
        release->setBounds(sustain->getRight(), starty, knobw, knobh);
    }
    else if (mode == 3) { // DAHDSR
        delay->setVisible(true);
        attack->setVisible(true);
        decay->setVisible(true);
        hold->setVisible(true);
        sustain->setVisible(true);
        release->setVisible(true);

        delay->setBounds((int)getX(), starty, knobw, knobh);
        attack->setBounds(delay->getRight(), starty, knobw, knobh);
        hold->setBounds(attack->getRight(), starty, knobw, knobh);
        decay->setBounds(hold->getRight(), starty, knobw, knobh);
        sustain->setBounds(decay->getRight(), starty, knobw, knobh);
        release->setBounds(sustain->getRight(), starty, knobw, knobh);
    }
    else { // ADSR
        int startX = int((getWidth() - 4 * knobw) / 2 - 2);
        attack->setVisible(true);
        decay->setVisible(true);
        sustain->setVisible(true);
        release->setVisible(true);

        attack->setBounds(bounds.getX() + startX, starty, knobw, knobh);
        decay->setBounds(attack->getRight(), starty, knobw, knobh);
        sustain->setBounds(decay->getRight(), starty, knobw, knobh);
        release->setBounds(sustain->getRight(), starty, knobw, knobh);
    }

    repaint();
}

void EnvDisplay::showEnvelopeModeMenu()
{
    auto mode = editor.audioProcessor.params.getRawParameterValue(envid + "_mode")->load();

    juce::PopupMenu menu;
    menu.addItem(1, "ADSR", true, mode == 0);
    menu.addItem(2, "AHD" , true, mode == 1);
    menu.addItem(3, "DADSR", true, mode == 2);
    menu.addItem(4, "DADHSR", true, mode == 3);

    auto menuPos = localPointToGlobal(modeBtn.getBounds().getBottomLeft());
    menu.setLookAndFeel(&getLookAndFeel());
    menu.showMenuAsync(juce::PopupMenu::Options()
        .withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
        .withTargetComponent(*this)
        .withTargetScreenArea({ menuPos.getX() - 50, menuPos.getY(), 1, 1 }),
        [this](int result) {
            if (result == 0) return;
            //editor.audioProcessor.undomgr->createUndo();
            auto param = editor.audioProcessor.params.getParameter(envid + "_mode");
            param->setValueNotifyingHost(param->convertTo0to1(float(result - 1)));
        });
}

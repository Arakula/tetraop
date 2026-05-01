#include "../../PluginEditor.h"
#include "../../Globals.h"
#include "LFODisplay.h"

LFODisplay::LFODisplay(TetraOPAudioProcessorEditor& e)
    : editor(e)
{
    startTimerHz(60);
    editor.audioProcessor.params.addParameterListener("lfo_grid", this);
    editor.audioProcessor.params.addParameterListener("lfo_grid_snap", this);

    juce::Desktop::getInstance().addGlobalMouseListener(this);

    addAndMakeVisible(gridBtn);
    addAndMakeVisible(rotLeftBtn);
    addAndMakeVisible(rotRightBtn);
    addAndMakeVisible(roundBtn);
    addAndMakeVisible(fileBtn);
    addAndMakeVisible(modeBtn);

    gridBtn.onClick = [this]()
        {
            showGridMenu();
        };

    rotLeftBtn.onClick = [this]
        {
            //editor.audioProcessor.undomgr->createUndo();
            auto& lfo = editor.audioProcessor.modulation->lfos[lfoidx];
            lfo.pattern.rotate(-1.f / display->gridX);
            lfo.pattern.buildSegments();
            repaint();
        };

    rotRightBtn.onClick = [this]
        {
            //editor.audioProcessor.undomgr->createUndo();
            auto& lfo = editor.audioProcessor.modulation->lfos[lfoidx];
            lfo.pattern.rotate(1.f / display->gridX);
            lfo.pattern.buildSegments();
            repaint();
        };

    roundBtn.onClick = [this]
        {
            //editor.audioProcessor.undomgr->createUndo();
            auto& lfo = editor.audioProcessor.modulation->lfos[lfoidx];
            bool isRoundCurve = lfo.pattern.points.size() && lfo.pattern.points[0].type > 1;
            for (auto& point : lfo.pattern.points) {
                point.type = isRoundCurve ? Pattern::Curve : Pattern::SinCurve;
            }
            lfo.pattern.buildSegments();
            repaint();
        };

    fileBtn.onClick = [this]
        {
            showFileMenu();
        };

    modeBtn.onClick = [this]
        {
            showModeMenu();
        };

    gridBtn.setAlpha(0.f);
    rotLeftBtn.setAlpha(0.f);
    rotRightBtn.setAlpha(0.f);
    roundBtn.setAlpha(0.f);
    modeBtn.setAlpha(0.f);
    fileBtn.setAlpha(0.f);

    rate = std::make_unique<Rotary>(editor, "lfo1_rate", "Rate", Rotary::Hz1f);
    rateSync = std::make_unique<Rotary>(editor, "lfo1_rate_sync", "Rate", Rotary::RateTempo, true);
    smooth = std::make_unique<Rotary>(editor, "lfo1_smooth", "Smth", Rotary::Percent);
    delay = std::make_unique<Rotary>(editor, "lfo1_delay", "Delay", Rotary::seconds1f);
    delaySync = std::make_unique<Rotary>(editor, "lfo1_delay_sync", "Delay", Rotary::DelayTempo);
    rise = std::make_unique<Rotary>(editor, "lfo1_rise", "Rise", Rotary::seconds1f);
    riseSync = std::make_unique<Rotary>(editor, "lfo1_rise_sync", "Rise", Rotary::DelayTempo);

    rate->radius = 14.f;
    rateSync->radius = 14.f;
    smooth->radius = 14.f;
    delay->radius = 14.f;
    delaySync->radius = 14.f;
    rise->radius = 14.f;
    riseSync->radius = 14.f;

    rate->yoffset -= 2;
    rateSync->yoffset -= 2;
    smooth->yoffset -= 2;
    delay->yoffset -= 2;
    delaySync->yoffset -= 2;
    rise->yoffset -= 2;
    riseSync->yoffset -= 2;

    addAndMakeVisible(rate.get());
    addAndMakeVisible(rateSync.get());
    addAndMakeVisible(smooth.get());
    addAndMakeVisible(delay.get());
    addAndMakeVisible(delaySync.get());
    addAndMakeVisible(rise.get());
    addAndMakeVisible(riseSync.get());

    addAndMakeVisible(syncBtn);
    syncBtn.setAlpha(0.f);
    syncBtn.onClick = [this]()
        {
            showSyncMenu();
        };

    display = std::make_unique<CurveEditor>(editor, &editor.audioProcessor.modulation->lfos[0].pattern, 
        5.f, COLOR_LFO(), COLOR_ACTIVE(), true, true);
    addAndMakeVisible(display.get());
    display->gridX = (int)editor.audioProcessor.params.getRawParameterValue("lfo_grid")->load();
    display->snap = (bool)editor.audioProcessor.params.getRawParameterValue("lfo_grid_snap")->load();
}

LFODisplay::~LFODisplay()
{
    juce::Desktop::getInstance().removeGlobalMouseListener(this);
    editor.audioProcessor.params.removeParameterListener("lfo_grid", this);
    editor.audioProcessor.params.removeParameterListener("lfo_grid_snap", this);
    if (lfoid.isNotEmpty()) {
        editor.audioProcessor.params.removeParameterListener(lfoid + "_mode", this);
        editor.audioProcessor.params.removeParameterListener(lfoid + "_sync", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_rate", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_rate_sync", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_smooth", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_delay", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_delay_sync", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_rise", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_rise_sync", this);
    }
}

void LFODisplay::mouseDown(const juce::MouseEvent& e)
{
    auto localPos = getLocalPoint(nullptr, e.getScreenPosition());
    if (!viewBounds.contains(localPos.toFloat())) {
        display->clearSelection();
    }
}

void LFODisplay::disconnect()
{
    if (lfoid.isNotEmpty()) {
        editor.audioProcessor.params.removeParameterListener(lfoid + "_mode", this);
        editor.audioProcessor.params.removeParameterListener(lfoid + "_sync", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_rate", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_rate_sync", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_smooth", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_delay", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_delay_sync", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_rise", this);
        //editor.audioProcessor.params.removeParameterListener(lfoid + "_rise_sync", this);
        editor.unregisterModParam(rate.get());
        editor.unregisterModParam(rateSync.get());
        editor.unregisterModParam(smooth.get());
        editor.unregisterModParam(delay.get());
        editor.unregisterModParam(delaySync.get());
        editor.unregisterModParam(rise.get());
        editor.unregisterModParam(riseSync.get());
    }
    lfoid = "";
}


void LFODisplay::timerCallback()
{
    auto selectedModId = juce::String(editor.audioProcessor.modulation->selectedMod);
    auto& displayMod = editor.audioProcessor.displayMod;

    if (selectedModId != lfoid && selectedModId.startsWith("lfo"))
    {
        connect(selectedModId);
        repaint();
    }
    else if (lfoid.isNotEmpty()) {
        auto& mod = editor.audioProcessor.modulation->modulators[lfoid];
        if (mod.active) {
            isActive = true;
            repaint(); // draw seek
        }
        else if (!mod.active && isActive) {
            isActive = false;
            display->repaint(); // repaint one last time to remove the seek
        }
    }
}

void LFODisplay::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "lfo_grid") {
        display->gridX = (int)newValue;
    }
    else if (parameterID == "lfo_grid_snap") {
        display->snap = (bool)newValue;
    }
    juce::MessageManager::callAsync([this]() { toggleUIComponents(); });
}

void LFODisplay::connect(juce::String id)
{
    disconnect();
    lfoid = id;
    lfoidx = lfoid.retainCharacters("0123456789").getIntValue() - 1;

    display->setPattern(&editor.audioProcessor.modulation->lfos[lfoidx].pattern);

    editor.audioProcessor.params.addParameterListener(lfoid + "_mode", this);
    editor.audioProcessor.params.addParameterListener(lfoid + "_sync", this);
    //editor.audioProcessor.params.addParameterListener(lfoid + "_rate", this);
    //editor.audioProcessor.params.addParameterListener(lfoid + "_rate_sync", this);
    //editor.audioProcessor.params.addParameterListener(lfoid + "_smooth", this);
    //editor.audioProcessor.params.addParameterListener(lfoid + "_delay", this);
    //editor.audioProcessor.params.addParameterListener(lfoid + "_delay_sync", this);
    //editor.audioProcessor.params.addParameterListener(lfoid + "_rise", this);
    //editor.audioProcessor.params.addParameterListener(lfoid + "_rise_sync", this);

    rate->setParamId(lfoid + "_rate");
    rateSync->setParamId(lfoid + "_rate_sync");
    smooth->setParamId(lfoid + "_smooth");
    delay->setParamId(lfoid + "_delay");
    delaySync->setParamId(lfoid + "_delay_sync");
    rise->setParamId(lfoid + "_rise");
    riseSync->setParamId(lfoid + "_rise_sync");

    editor.registerModParam(rate.get());
    editor.registerModParam(rateSync.get());
    editor.registerModParam(smooth.get());
    editor.registerModParam(delay.get());
    editor.registerModParam(delaySync.get());
    editor.registerModParam(rise.get());
    editor.registerModParam(riseSync.get());

    toggleUIComponents();
    editor.audioProcessor.modulation->UIDirty.store(true); // refresh connections
}

void LFODisplay::paint(juce::Graphics& g)
{
    if (lfoid.isEmpty() || !isVisible()) return;

    // draw sync btn
    auto sync = (int)editor.audioProcessor.params.getRawParameterValue(lfoid + "_sync")->load();
    if (sync == 0) {
        UIUtils::drawClock(g, syncBtn.getBounds().reduced(5).toFloat(), Colour(0xff333333));
    }
    else {
        UIUtils::drawNote(g, syncBtn.getBounds().reduced(5).toFloat(), sync - 1, Colour(0xff333333));
    }

    // draw top view buttons
    g.setFont(juce::FontOptions(12.f));
    g.setColour(COLOR_VIEWPORT_TEXT());
    g.drawText("Grid " + juce::String(display->gridX), gridBtn.getBounds(), juce::Justification::centredLeft);
    g.drawText("File ", fileBtn.getBounds(), juce::Justification::centredLeft);
    UIUtils::drawTriangle(g, rotLeftBtn.getBounds().toFloat().reduced(4.f), 3, Colours::white.withAlpha(0.5f));
    UIUtils::drawTriangle(g, rotRightBtn.getBounds().toFloat().reduced(4.f), 1, Colours::white.withAlpha(0.5f));

    auto& lfo = editor.audioProcessor.modulation->lfos[lfoidx];
    auto isRoundWave = lfo.pattern.points.size() && lfo.pattern.points[0].type > 1;
    UIUtils::drawSineWave(g, roundBtn.getBounds().toFloat().reduced(0.f, 3.f), 2, isRoundWave ? COLOR_LFO() : Colour(0xff333333));
    UIUtils::drawTriangle(g, fileBtn.getBounds().withTrimmedLeft(23).toFloat().reduced(5.f), 2, Colour(0xff333333));

    g.setColour(Colour(0xff333333));
    auto mode = (LFO::Mode)editor.audioProcessor.params.getRawParameterValue(lfoid + "_mode")->load();
    auto modestr = mode == LFO::Mode::Trigger ? "Trig"
        : mode == LFO::Mode::Sync ? "Sync"
        : "Env";
    g.drawText(modestr, modeBtn.getBounds(), juce::Justification::centredLeft);
    UIUtils::drawTriangle(g, modeBtn.getBounds().withTrimmedLeft(23).toFloat().reduced(5.f), 2, Colour(0xff333333));

    auto& mod = editor.audioProcessor.modulation->modulators[lfoid];
    display->drawSeek = mod.active && mod.connections;
    display->seekPos = lfo.getXNorm(mod.x, mod.x_offset);
    display->repaint();
}

void LFODisplay::resized()
{
    viewBounds = getLocalBounds().toFloat().withTrimmedBottom(globals::KNOB_HEIGHT).withTrimmedTop(15.f);
    display->setBounds(viewBounds.toNearestInt());
    toggleUIComponents();
}

void LFODisplay::toggleUIComponents()
{
    if (lfoid.isEmpty())
        return;

    isSync = (bool)editor.audioProcessor.params.getRawParameterValue(lfoid + "_sync")->load();
    rate->setVisible(!isSync);
    rateSync->setVisible(isSync);
    delay->setVisible(!isSync);
    delaySync->setVisible(isSync);
    rise->setVisible(!isSync);
    riseSync->setVisible(isSync);

    auto bounds = getLocalBounds();
    auto knobw = int(globals::KNOB_WIDTH * 0.9f);
    auto knobh = int(globals::KNOB_HEIGHT * 0.9f);
    smooth->setBounds(bounds.getCentreX() + knobw, bounds.getBottom() - globals::KNOB_HEIGHT, knobw, knobh);
    rise->setBounds(bounds.getCentreX(), bounds.getBottom() - globals::KNOB_HEIGHT, knobw, knobh);
    delay->setBounds(bounds.getCentreX() - knobw, bounds.getBottom() - globals::KNOB_HEIGHT, knobw, knobh);
    rate->setBounds(bounds.getCentreX() - knobw * 2, bounds.getBottom() - globals::KNOB_HEIGHT, knobw, knobh);

    delaySync->setBounds(delay->getBounds());
    riseSync->setBounds(rise->getBounds());
    rateSync->setBounds(rate->getBounds());

    gridBtn.setBounds((int)viewBounds.reduced(5.f).getX(), (int)viewBounds.reduced(5.f).getY() - 16, 40, 15);
    rotLeftBtn.setBounds(gridBtn.getBounds().withX(gridBtn.getRight() + 5).withWidth(15));
    rotRightBtn.setBounds(rotLeftBtn.getBounds().withX(rotLeftBtn.getRight() + 5));
    roundBtn.setBounds(rotRightBtn.getBounds().withX(rotRightBtn.getRight() + 10).withWidth(25));
    modeBtn.setBounds(gridBtn.getBounds().withWidth(40).withRightX((int)(viewBounds.reduced(5.f).getX() + viewBounds.reduced(5.f).getWidth())));
    fileBtn.setBounds(modeBtn.getBounds().translated(-50, 0).withWidth(40));

    syncBtn.setBounds(bounds.getX() + 5, bounds.getBottom() - globals::KNOB_HEIGHT + 10, 25, 25);

    repaint();
}

void LFODisplay::showGridMenu()
{
    juce::PopupMenu menu;
    for (int i = 2; i <= 16; ++i) {
        menu.addItem(i, juce::String(i), true, display->gridX == i);
    }

    auto menuPos = localPointToGlobal(gridBtn.getBounds().getBottomLeft());
    menu.setLookAndFeel(&getLookAndFeel());
    menu.showMenuAsync(juce::PopupMenu::Options()
        .withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
        .withTargetComponent(*this)
        .withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
        [this](int result) {
            if (result == 0) return;
            auto param = editor.audioProcessor.params.getParameter("lfo_grid");
            param->setValueNotifyingHost(param->convertTo0to1(float(result)));
        });
}

void LFODisplay::showModeMenu()
{
    auto mode = editor.audioProcessor.modulation->lfos[lfoidx].mode;
    juce::PopupMenu menu;
    menu.addItem(1, "Trigger", true, mode == LFO::Trigger);
    menu.addItem(2, "Sync", true, mode == LFO::Sync);
    menu.addItem(3, "Envelope", true, mode == LFO::Envelope);

    auto menuPos = localPointToGlobal(modeBtn.getBounds().getBottomLeft());
    menu.setLookAndFeel(&getLookAndFeel());
    menu.showMenuAsync(juce::PopupMenu::Options()
        .withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
        .withTargetComponent(*this)
        .withTargetScreenArea({ menuPos.getX() - 70, menuPos.getY(), 1, 1 }),
        [this](int result) {
            if (result == 0) return;
            //editor.audioProcessor.undomgr->createUndo();
            auto param = editor.audioProcessor.params.getParameter("lfo" + juce::String(lfoidx + 1) + "_mode");
            param->setValueNotifyingHost(param->convertTo0to1(result - 1.f));
        });
}

void LFODisplay::showSyncMenu()
{
    auto sync = (int)editor.audioProcessor.params.getRawParameterValue(lfoid + "_sync")->load();
    juce::PopupMenu menu;
    menu.addItem(1, "Hertz", true, sync == LFO::Rate);
    menu.addItem(2, "Straight", true, sync == LFO::Straight);
    menu.addItem(3, "Triplet", true, sync == LFO::Tripplet);
    menu.addItem(4, "Dotted", true, sync == LFO::Dotted);

    auto menuPos = localPointToGlobal(syncBtn.getBounds().getBottomLeft());
    menu.setLookAndFeel(&getLookAndFeel());
    menu.showMenuAsync(juce::PopupMenu::Options()
        .withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
        .withTargetComponent(*this)
        .withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
        [this](int result) {
            if (result == 0) return;
            //editor.audioProcessor.undomgr->createUndo();
            auto param = editor.audioProcessor.params.getParameter(lfoid + "_sync");
            param->setValueNotifyingHost(param->convertTo0to1((float)(result - 1)));
        });
}

void LFODisplay::loadLfoFile(const juce::File& file)
{
    auto json = juce::JSON::parse(file);
    auto* array = json.getArray();
    if (array == nullptr)
        return;

    //editor.audioProcessor.undomgr->createUndo();
    auto& lfo = editor.audioProcessor.modulation->lfos[lfoidx];
    lfo.pattern.clear();
    // Insert points in reverse order. Pattern::insertPoint uses lower_bound,
    // so new points are placed before existing points that share the same t.
    // Iterating backwards ends up restoring the file's original order, which
    // matters for waveforms that use two points at the same t to create
    // instantaneous jumps (saw, square, gates, sequencers...).
    for (int i = array->size() - 1; i >= 0; --i) {
        const auto& ptVar = array->getReference(i);
        auto t = (float)(double)ptVar.getProperty("t", 0.0);
        auto v = (float)(double)ptVar.getProperty("v", 0.0);
        auto c = (float)(double)ptVar.getProperty("c", 0.0);
        lfo.pattern.insertPoint(t, v, c, Pattern::Curve);
    }
    lfo.pattern.separatePointsWithSameX(); // FIX
    lfo.pattern.buildSegments();
    repaint();
}

void LFODisplay::buildFileSubmenu(juce::PopupMenu& menu, const juce::File& folder)
{
    if (!folder.isDirectory())
        return;

    // loose JSON files at the top of the folder
    juce::Array<juce::File> looseFiles;
    folder.findChildFiles(looseFiles, juce::File::findFiles, false, "*.json");
    std::sort(looseFiles.begin(), looseFiles.end(),
        [](const juce::File& a, const juce::File& b) {
            return a.getFileNameWithoutExtension().compareIgnoreCase(b.getFileNameWithoutExtension()) < 0;
        });
    for (const auto& file : looseFiles) {
        menu.addItem(file.getFileNameWithoutExtension(),
            [this, file] { loadLfoFile(file); });
    }

    // subdirectories as submenus
    juce::Array<juce::File> subDirs;
    folder.findChildFiles(subDirs, juce::File::findDirectories, false);
    std::sort(subDirs.begin(), subDirs.end(),
        [](const juce::File& a, const juce::File& b) {
            return a.getFileName().compareIgnoreCase(b.getFileName()) < 0;
        });

    if (!looseFiles.isEmpty() && !subDirs.isEmpty())
        menu.addSeparator();

    for (const auto& dir : subDirs) {
        juce::Array<juce::File> jsonFiles;
        dir.findChildFiles(jsonFiles, juce::File::findFiles, false, "*.json");
        if (jsonFiles.isEmpty())
            continue;

        std::sort(jsonFiles.begin(), jsonFiles.end(),
            [](const juce::File& a, const juce::File& b) {
                return a.getFileNameWithoutExtension().compareIgnoreCase(b.getFileNameWithoutExtension()) < 0;
            });

        juce::PopupMenu subMenu;
        for (const auto& file : jsonFiles) {
            subMenu.addItem(file.getFileNameWithoutExtension(),
                [this, file] { loadLfoFile(file); });
        }
        menu.addSubMenu(dir.getFileName(), subMenu);
    }
}

void LFODisplay::saveLfoFile(const juce::String& /*name*/)
{
    //auto fname = juce::File::createLegalFileName(name);
    //if (fname.isEmpty())
    //    return;
    //
    //auto userLfosDir = global::userContentFolder.getChildFile("LFOs");
    //auto result = userLfosDir.createDirectory();
    //if (result.failed())
    //    return;
    //
    //auto file = userLfosDir.getChildFile(fname + ".json");
    //
    //auto& lfo = editor.audioProcessor.modulation->lfos[lfoidx];
    //juce::Array<juce::var> array;
    //for (const auto& point : lfo.pattern.points) {
    //    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    //    obj->setProperty("t", (double)point.x);
    //    obj->setProperty("v", (double)point.y);
    //    obj->setProperty("c", (double)point.tension);
    //    array.add(juce::var(obj.get()));
    //}
    //
    //file.replaceWithText(juce::JSON::toString(juce::var(array)));
}

void LFODisplay::showFileMenu()
{
    juce::PopupMenu menu;

    //auto userLfosDir = global::userContentFolder.getChildFile("LFOs");
    //if (userLfosDir.isDirectory()) {
    //    juce::PopupMenu userMenu;
    //    buildFileSubmenu(userMenu, userLfosDir);
    //    if (userMenu.getNumItems() > 0) {
    //        menu.addSubMenu("User", userMenu);
    //        menu.addSeparator();
    //    }
    //}
    //
    //buildFileSubmenu(menu, global::contentPath.getChildFile("LFOs"));
    //
    //if (menu.getNumItems() > 0)
    //    menu.addSeparator();
    //
    //menu.addItem("Save As...", [this] {
    //    auto defaultName = juce::String("LFO ") + juce::String(lfoidx + 1);
    //    editor.showSaveLfoDialog(defaultName, [this](const juce::String& name) {
    //        saveLfoFile(name);
    //    });
    //});
    //
    //menu.addSeparator();

    menu.addItem("Copy", [this] {
        editor.audioProcessor.modulation->lfos[lfoidx].pattern.copy();
    });

    menu.addItem("Paste", [this] {
        //editor.audioProcessor.undomgr->createUndo();
        auto& lfo = editor.audioProcessor.modulation->lfos[lfoidx];
        lfo.pattern.paste();
        lfo.pattern.buildSegments();
        repaint();
    });

    auto menuPos = localPointToGlobal(fileBtn.getBounds().getBottomLeft());
    menu.setLookAndFeel(&getLookAndFeel());
    menu.showMenuAsync(juce::PopupMenu::Options()
        .withParentComponent(findParentComponentOfClass<juce::AudioProcessorEditor>())
        .withTargetComponent(*this)
        .withTargetScreenArea({ menuPos.getX() - 70, menuPos.getY(), 1, 1 }));
}

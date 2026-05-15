// Copyright 2025 tilr

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Globals.h"

TetraOPAudioProcessorEditor::TetraOPAudioProcessorEditor(TetraOPAudioProcessor& p)
    : AudioProcessorEditor(p)
    , audioProcessor(p)
{
    startTimerHz(60);

    if (!globals::themeLoaded) {
        loadTheme();
    }

    setSize(KNOB_WIDTH * (2+2+2+2 + 3) +KNOB_WIDTH_SM * 6 + PANEL_PAD * 4 + int(FILTER_PANEL_HMARGIN * 2.5), 620 + HEADER_HEIGHT + PANEL_PAD);
    buildUI();
    selectTab(audioProcessor.selectedTab);
}

TetraOPAudioProcessorEditor::~TetraOPAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    juce::Desktop::getInstance().removeGlobalMouseListener(this);
}

void TetraOPAudioProcessorEditor::buildUI()
{
    setLookAndFeel(nullptr);
    customLookAndFeel = std::make_unique<CustomLookAndFeel>();
    customLookAndFeel->scale = audioProcessor.scale;
    setLookAndFeel(customLookAndFeel.get());

    if (auto* wrapper = dynamic_cast<ScaledPluginEditor*>(audioProcessor.getActiveEditor()))
        wrapper->setLookAndFeel(customLookAndFeel.get());

    header = std::make_unique<Header>(*this);
    addAndMakeVisible(header.get());
    header->setBounds(0, 0, getWidth(), HEADER_HEIGHT);

    oscA = std::make_unique<OSCPanel>(*this, 0);
    addAndMakeVisible(oscA.get());
    oscA->setBounds(PANEL_PAD, PANEL_PAD + HEADER_HEIGHT, KNOB_WIDTH * 4 + KNOB_WIDTH_SM * 3, KNOB_HEIGHT * 2 + PANEL_HEADER_HEIGHT + 7);

    oscB = std::make_unique<OSCPanel>(*this, 1);
    addAndMakeVisible(oscB.get());
    oscB->setBounds(oscA->getBounds().translated(oscA->getWidth() + PANEL_PAD, 0));

    oscC = std::make_unique<OSCPanel>(*this, 2);
    addAndMakeVisible(oscC.get());
    oscC->setBounds(oscA->getBounds().translated(0, oscA->getHeight() + PANEL_PAD));

    oscD = std::make_unique<OSCPanel>(*this, 3);
    addAndMakeVisible(oscD.get());
    oscD->setBounds(oscA->getBounds().translated(oscA->getWidth() + PANEL_PAD, oscA->getHeight() + PANEL_PAD));

    filter1 = std::make_unique<FilterPanel>(*this, 0);
    addAndMakeVisible(filter1.get());
    filter1->setBounds(oscB->getBounds().translated(oscB->getWidth() + PANEL_PAD, 0).withWidth(KNOB_WIDTH * 3 + int(FILTER_PANEL_HMARGIN * 2.5)));

    filter2 = std::make_unique<FilterPanel>(*this, 1);
    addAndMakeVisible(filter2.get());
    filter2->setBounds(filter1->getBounds().translated(0, filter1->getHeight() + PANEL_PAD));

    macros = std::make_unique<MacrosPanel>(*this);
    addAndMakeVisible(macros.get());
    macros->setBounds(Rectangle<int>(oscA->getX(), oscD->getBottom() + PANEL_PAD, 75, 10)
        .withBottom(getBottom() - PANEL_PAD));

    envelopes = std::make_unique<EnvDisplay>(*this);
    addAndMakeVisible(envelopes.get());
    envelopes->setBounds(Rectangle<int>(oscA->getX() + 75 + PANEL_PAD, oscD->getBottom() + PANEL_PAD, oscA->getWidth() - 75 - PANEL_PAD, 10)
        .withBottom(getBottom() - PANEL_PAD));

    lfos = std::make_unique<LFODisplay>(*this);
    addAndMakeVisible(lfos.get());
    lfos->setBounds(envelopes->getBounds().translated(envelopes->getWidth() + PANEL_PAD, 0));

    mods = std::make_unique<ModulatorsPanel>(*this);
    addAndMakeVisible(mods.get());
    mods->setBounds(Rectangle<int>(lfos->getRight() + PANEL_PAD, lfos->getY(), 10, lfos->getHeight())
        .withRight(oscB->getRight()));

    globals = std::make_unique<GlobalsPanel>(*this);
    addAndMakeVisible(globals.get());
    globals->setVisible(!audioProcessor.fmMatrixVisible);
    globals->setBounds(filter2->getBounds().withY(filter2->getBottom() + PANEL_PAD).withBottom(getHeight() - PANEL_PAD));

    fmMatrix = std::make_unique<FmMatrixPanel>(*this);
    addAndMakeVisible(fmMatrix.get());
    fmMatrix->setVisible(audioProcessor.fmMatrixVisible);
    fmMatrix->setBounds(globals->getBounds());

    matrixPanel = std::make_unique<MatrixPanel>(*this);
    addChildComponent(matrixPanel.get());
    matrixPanel->setBounds(Rectangle<int>(1,1)
        .withX(oscA->getX())
        .withY(oscA->getY())
        .withRight(filter2->getRight())
        .withBottom(filter2->getBottom()));

    configsPanel = std::make_unique<ConfigsPanel>(*this);
    addChildComponent(configsPanel.get());
    configsPanel->setBounds(matrixPanel->getBounds());

    fxPanel = std::make_unique<FXPanel>(*this);
    addChildComponent(fxPanel.get());
    fxPanel->setBounds(matrixPanel->getBounds());

    dragDropOverlay = std::make_unique<DragDropOverlay>();
    addChildComponent (dragDropOverlay.get());
    dragDropOverlay->setInterceptsMouseClicks(false, false);
    dragDropOverlay->setBounds(0, 0, getWidth(), getHeight());

    aboutDialog = std::make_unique<AboutDialog>();
    addChildComponent(aboutDialog.get());
    aboutDialog->setBounds(0, 0, getWidth(), getHeight());

    juce::Desktop::getInstance().addGlobalMouseListener(this);
    audioProcessor.modulation->UIDirty.store(true); // refresh connections on startup
}

void TetraOPAudioProcessorEditor::toggleFmMatrix()
{
    audioProcessor.fmMatrixVisible = !audioProcessor.fmMatrixVisible;
    toggleUIComponents();
}

void TetraOPAudioProcessorEditor::toggleUIComponents()
{
    globals->setVisible(!audioProcessor.fmMatrixVisible);
    fmMatrix->setVisible(audioProcessor.fmMatrixVisible);
}

void TetraOPAudioProcessorEditor::rebuild()
{
    PopupMenu::dismissAllActiveMenus();
    modulatedParams.clear();
    juce::Desktop::getInstance().removeGlobalMouseListener(this);

    removeAllChildren();

    if (auto* wrapper = dynamic_cast<ScaledPluginEditor*>(audioProcessor.getActiveEditor()))
        wrapper->setLookAndFeel(nullptr);

    buildUI();
    selectTab(audioProcessor.selectedTab);

    resized();
    repaint();
}

void TetraOPAudioProcessorEditor::showAboutDialog()
{
    aboutDialog->setVisible(true);
}

void TetraOPAudioProcessorEditor::loadTheme() const
{
    globals::loadDefaultTheme();
    // audioProcessor.modulation->setSelectedMod("env1"); // FIX
}

void TetraOPAudioProcessorEditor::timerCallback()
{
    if (isDragDropModulation) {
        dragDropOverlay->repaint();
    }

    auto connections = audioProcessor.modulation->getConnections();
    auto voiceActive = audioProcessor.modulation->isAnyVoiceActive();

    // refresh modulation values for all connected params
    for (auto& conn : connections) {
        if (modulatedParams.find(conn.dst) != modulatedParams.end()) {
            auto param = modulatedParams[conn.dst];
            if (param.ref->isShowing()) {
                auto val = audioProcessor.modulation->getModulatedNormSafe(conn.dst);
                if (param.ref->modValue != val || param.ref->voiceActive != voiceActive) { // save on repaints
                    param.ref->modValue = val;
                    param.ref->voiceActive = voiceActive;
                    param.ref->repaint();
                }
            }
        }
    }

    // handle new modulation connects or disconnects
    // also handle selected modulator change
    if (audioProcessor.modulation->UIDirty.exchange(false)) {
        if (matrixPanel->isVisible()) {
            matrixPanel->setConnections(connections);
        }

        std::unordered_set<juce::String> modulated;
        for (auto& conn : connections) {
            modulated.insert(conn.dst);
        }

        // handle connects and disconnects
        for (auto& [paramId, param] : modulatedParams) {
            param.ref->modulated = modulated.count(paramId) > 0;
            param.ref->setModId(""); // start by disabling current modulator for every param
        }

        auto& selmod = audioProcessor.modulation->selectedMod;
        auto selmodColor = String(selmod).startsWith("env")
            ? COLOR_ENVELOPE()
            : String(selmod).startsWith("lfo")
            ? COLOR_LFO()
            : String(selmod).startsWith("macro")
            ? COLOR_MACRO()
            : String(selmod).startsWith("rnd")
            ? COLOR_RND()
            : COLOR_OTHER_MOD();

        // handle selected modulator change
        // when modId is set, the mod range is displayed and editable in the UI
        for (auto& conn : connections) {
            if (modulatedParams.find(conn.dst) != modulatedParams.end()) {
                auto param = modulatedParams[conn.dst];
                if (conn.src == selmod) {
                    // if the param is modulated by the selected mod, activate it
                    param.ref->setModId(juce::String("mod") + juce::String(conn.id) + "_amt");
                    param.ref->modBipolar = conn.bipolar;
                    param.ref->modColor = conn.bypass ? Colours::transparentBlack : selmodColor;
                }
            }
        }

        // finally repaint all params
        for (auto& [paramId, param] : modulatedParams) {
            if (param.ref->isShowing()) {
                param.ref->repaint();
            }
        }
    }
}

void TetraOPAudioProcessorEditor::startDragDrop(String modID, juce::Component* component)
{
    (void)modID;
    (void)component;

    auto modColor = modID.startsWith("env")
        ? COLOR_ENVELOPE()
        : modID.startsWith("lfo")
        ? COLOR_LFO()
        : modID.startsWith("macro")
        ? COLOR_MACRO()
        : modID.startsWith("rnd")
        ? COLOR_RND()
        : COLOR_OTHER_MOD();

    isDragDropModulation = true;
    dragDropModID = modID;
    dragDropOverlay->setVisible(true);
    dragDropOverlay->arrowStart = dragDropOverlay->getLocalPoint(component, Point<int>{10, 10}).toFloat();
    dragDropOverlay->arrowEnd = dragDropOverlay->arrowStart;
    dragDropOverlay->arrowColor = modColor;

    auto connections = audioProcessor.modulation->getConnections();
    std::unordered_set<juce::String> modulated;
    for (auto& conn : connections) {
        if (conn.src == modID) {
            modulated.insert(conn.dst);
        }
    }

    for (auto& [paramId, param] : modulatedParams) {
        if (param.ref->isShowing() && modulated.count(paramId) == 0) {
            param.ref->showDragAndDrop = true;
            param.ref->dragAndDropColour = modColor.withAlpha(0.5f);
            param.ref->repaint();
        }
    }
}

/*
* On global mouse up finish drag and drop of modulators
*/
void TetraOPAudioProcessorEditor::mouseUp(const juce::MouseEvent& e)
{
    if (isDragDropModulation) {
        isDragDropModulation = false;
        dragDropOverlay->setVisible(false);

        audioProcessor.modulation->setSelectedMod(dragDropModID.toStdString());
        dragDropModID = "";

        for (auto& [paramId, param] : modulatedParams)
            if (param.ref->showDragAndDrop) {
                param.ref->showDragAndDrop = false;
                param.ref->repaint(); // unpaint drag and drop
            }

        auto* comp = getComponentAt(e.getEventRelativeTo(this).getPosition());
        if (comp == nullptr || !comp->isEnabled())
            return;

        auto id = comp->getName();
        audioProcessor.undomgr->createUndo();

        // for each modulatable param turn off drag and drop
        // if the drag and drop finished on the param make a new connection
        for (auto& [paramId, param] : modulatedParams) {
            if (param.ref->getName() == id) {
                audioProcessor.modulation->connect(dragDropModID.toStdString(), id.toStdString());
            }
        }
    }
}

/*
* Shortcut to connect a param with current selected modulator on mouse down with control pressed
* Allows to quickly create connections without using drag and drop
*/
void TetraOPAudioProcessorEditor::quickConnect(String paramId)
{
    if (!modulatedParams.count(paramId.toStdString()))
        return;

    audioProcessor.undomgr->createUndo();

    audioProcessor.modulation->connect(
        audioProcessor.modulation->selectedMod,
        paramId.toStdString()
    );

    auto conn = audioProcessor.modulation->getConnection(
        audioProcessor.modulation->selectedMod,
        paramId.toStdString()
    );

    if (conn == nullptr)
        return;

    // reset connection amount to zero
    auto modSliderId = "mod" + String(conn->id) + "_amt";
    audioProcessor.params.getParameter(modSliderId)->setValueNotifyingHost(0.5f); // 0.5f norm is zero value
    auto& param = modulatedParams[paramId.toStdString()];
    param.ref->modId = modSliderId;
}

void TetraOPAudioProcessorEditor::setMouseHoverParam(ModulatedParam* param)
{
    mouseHoverParam = param;
}

void TetraOPAudioProcessorEditor::registerModParam(ModulatedParam* param, ParamCategory cat)
{
    modulatedParams[param->paramId] = { param, cat };
}

void TetraOPAudioProcessorEditor::unregisterModParam(ModulatedParam* param)
{
    modulatedParams.erase(param->paramId);
}

void TetraOPAudioProcessorEditor::selectTab(int tab)
{
    fxPanel->setVisible(tab == 1);
    matrixPanel->setVisible(tab == 2);
    configsPanel->setVisible(tab == 3);
    audioProcessor.selectedTab = tab;
    header->repaint();

    if (matrixPanel->isVisible()) {
        audioProcessor.modulation->UIDirty.store(true); // trigger a connection refresh
    }

    repaint();
}

void TetraOPAudioProcessorEditor::parameterChanged(const juce::String&, float)
{
    MessageManager::callAsync([this] { toggleUIComponents(); });
};

void TetraOPAudioProcessorEditor::paint(Graphics& g)
{
    g.fillAll(COLOR_BACKGROUND());
}

void TetraOPAudioProcessorEditor::showParamContextMenu(ModulatedParam* param)
{
    (void)param;
    /*
    PopupMenu menu;
    menu.addItem(1, "Enter value");
    menu.addItem(2, "Reset value");

    auto connections = audioProcessor.modulation->getConnections();
    std::vector <Modulation::Connection> paramConns;
    for (auto& conn : connections) {
        if (conn.dst == param->paramId) {
            paramConns.push_back(conn);
        }
    }

    PopupMenu removeMods;
    PopupMenu bypassMods;

    if (paramConns.size()) {
        removeMods.addItem(200, "All");
        bypassMods.addItem(300, "All");

        auto i = 1;
        for (auto& conn : paramConns) {
            removeMods.addItem(200 + i, UIUtils::formatName(conn.src));
            bypassMods.addItem(300 + i, UIUtils::formatName(conn.src), true, conn.bypass);
            i += 1;
        }

        menu.addSeparator();
        menu.addSubMenu("Remove mod", removeMods);
        menu.addSubMenu("Bypass mod", bypassMods);
    }

    auto menuPos = param->localPointToGlobal(param->getLocalBounds().getCentre());
    auto paramPos = getLocalPoint(param, param->getLocalBounds().getCentre());
    menu.showMenuAsync(PopupMenu::Options()
        .withTargetComponent(*this)
        .withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
        [this, paramConns, param, paramPos](int result) {
            if (result == 0) return;
            else if (result == 1) {
                floatInputOverlay = std::make_unique<FloatInput>(paramPos.x, paramPos.y, [this, param](float value)
                    {
                        auto p = audioProcessor.params.getParameter(param->paramId);
                        if (auto* rotary = dynamic_cast<Rotary*>(param)) {
                            if (rotary->format == Rotary::Percent || rotary->format == Rotary::ABMix)
                                value *= 0.01f;
                            else if (rotary->format == Rotary::FilterLPHP) {
                                const float denom = std::log10(1000.f);
                                float x = std::log10(value / 20.0f) / denom;
                                if (x > 1.0f)  x = 1.0f;
                                if (x < 0.0f)  x = 0.0f;
                                value = (value >= 20.f && value <= 20000.f)
                                    ? x : (x - 1.0f);
                            }
                        }
                        else if (auto* vslider = dynamic_cast<VSlider*>(param)){
                            value = std::pow(10.0f, value / 20.0f);
                        }
                        else if (auto* hslider = dynamic_cast<HSlider*>(param)) {
                            if (hslider->format == HSlider::dB) {
                                value = std::pow(10.0f, value / 20.0f);
                            }
                            else {
                                value *= 0.01f;
                            }
                        }

                        p->setValueNotifyingHost(p->convertTo0to1(value));
                        removeChildComponent(floatInputOverlay.get());
                    }, [this]()
                    {
                        removeChildComponent(floatInputOverlay.get());
                    });
                floatInputOverlay->setBounds({ 0, 0, getWidth(), getHeight() });
                addAndMakeVisible(floatInputOverlay.get());
                floatInputOverlay->focus();
            }
            else if (result == 2) {
                auto p = audioProcessor.params.getParameter(param->paramId);
                p->setValueNotifyingHost(p->getDefaultValue());
            }
            else if (result == 200) {
                for (auto& conn : paramConns) {
                    audioProcessor.modulation->disconnect(conn.src, conn.dst);
                }
            }
            else if (result == 300) {
                bool allBypassed = true;
                for (auto& conn : paramConns) {
                    if (!conn.bypass) {
                        allBypassed = false;
                        break;
                    }
                }
                for (auto& conn : paramConns) {
                    audioProcessor.modulation->setConnectionBypass(conn.src, conn.dst, !allBypassed);
                }
            }
            else if (result > 200 && result < 300) {
                auto index = result - 200 - 1;
                auto& conn = paramConns[index];
                audioProcessor.modulation->disconnect(conn.src, conn.dst);
            }
            else if (result > 300 && result < 400) {
                auto index = result - 300 - 1;
                auto& conn = paramConns[index];
                audioProcessor.modulation->setConnectionBypass(conn.src, conn.dst, !conn.bypass);
            }
        });
        */
}

/*
void TetraOPAudioProcessorEditor::showModContextMenu(Modulator* mod)
{
    (void)mod;
    PopupMenu menu;

    auto connections = audioProcessor.modulation->getConnections();
    std::vector <Modulation::Connection> paramConns;
    for (auto& conn : connections) {
        if (conn.src == mod->modId) {
            paramConns.push_back(conn);
        }
    }

    PopupMenu removeMods;
    PopupMenu bypassMods;

    if (paramConns.size()) {
        removeMods.addItem(200, "All");
        bypassMods.addItem(300, "All");

        auto i = 1;
        for (auto& conn : paramConns) {
            removeMods.addItem(200 + i, UIUtils::formatName(conn.dst));
            bypassMods.addItem(300 + i, UIUtils::formatName(conn.dst), true, conn.bypass);
            i += 1;
        }

        menu.addSeparator();
        menu.addSubMenu("Remove mod", removeMods);
        menu.addSubMenu("Bypass mod", bypassMods);
    }

    auto menuPos = mod->localPointToGlobal(mod->getLocalBounds().getCentre());
    menu.showMenuAsync(PopupMenu::Options()
        .withTargetComponent(*this)
        .withTargetScreenArea({ menuPos.getX(), menuPos.getY(), 1, 1 }),
        [this, mod, paramConns](int result) {
            if (result == 0) return;

            if (result == 200) {
                for (auto& conn : paramConns) {
                    audioProcessor.modulation->disconnect(conn.src, conn.dst);
                }
            }
            else if (result == 300) {
                bool allBypassed = true;
                for (auto& conn : paramConns) {
                    if (!conn.bypass) {
                        allBypassed = false;
                        break;
                    }
                }
                for (auto& conn : paramConns) {
                    audioProcessor.modulation->setConnectionBypass(conn.src, conn.dst, !allBypassed);
                }
            }
            else if (result > 200 && result < 300) {
                auto index = result - 200 - 1;
                auto& conn = paramConns[index];
                audioProcessor.modulation->disconnect(conn.src, conn.dst);
            }
            else if (result > 300 && result < 400) {
                auto index = result - 300 - 1;
                auto& conn = paramConns[index];
                audioProcessor.modulation->setConnectionBypass(conn.src, conn.dst, !conn.bypass);
            }
        });
    }
*/

void TetraOPAudioProcessorEditor::showMacroRename(Macro* macro)
{
    (void)macro;
    //auto paramPos = getLocalPoint(macro, macro->getLocalBounds().getCentre());
    //textInputOverlay = std::make_unique<TextInput>(paramPos.x, paramPos.y, [this, macro](String text)
    //    {
    //        if (text.isEmpty()) {
    //            text = String("Macro") + String(macro->index);
    //        }
    //        audioProcessor.modulation->macroNames[macro->index] = text;
    //        removeChildComponent(textInputOverlay.get());
    //    }, [this]()
    //        {
    //            removeChildComponent(textInputOverlay.get());
    //        });
    //    textInputOverlay->setBounds({ 0, 0, getWidth(), getHeight() });
    //    addAndMakeVisible(textInputOverlay.get());
    //    textInputOverlay->focus();
    //}
}
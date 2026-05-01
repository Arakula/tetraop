// Copyright 2025 tilr

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Globals.h"

TetraOPAudioProcessorEditor::TetraOPAudioProcessorEditor(TetraOPAudioProcessor& p)
    : AudioProcessorEditor(p)
    , audioProcessor(p)
{
    setSize(KNOB_WIDTH * (2+2+2+2 + 3) +KNOB_WIDTH_SM * 6 + PANEL_PAD * 4 + int(FILTER_PANEL_HMARGIN * 2.5), 620);
    Desktop::getInstance().setGlobalScaleFactor(audioProcessor.scale);
    startTimerHz(60);

    if (!globals::themeLoaded) {
        loadTheme();
    }

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

    aboutDialog = std::make_unique<AboutDialog>();
    addChildComponent(aboutDialog.get());
    aboutDialog->setBounds(0, 0, getWidth(), getHeight());

    resizeCorner = std::make_unique<ResizeCorner>();
    addAndMakeVisible(resizeCorner.get());
    resizeCorner->setBounds(getWidth() - 15, getHeight() - 15, 15, 15);
    resizeCorner->onDrag = ([this](int distX, int distY)
        {
            if (!resizing) {
                resizing = true;
                resizeStart = audioProcessor.scale;
            }
            auto w = getWidth() * resizeStart;
            auto h = getHeight() * resizeStart;
            auto ratioW = resizeStart + distX / w;
            auto ratioH = resizeStart + distY / h;

            resizeRatio = std::max(std::max(ratioW, ratioH), 0.75f);
            Desktop::getInstance().setGlobalScaleFactor(resizeRatio);
            customLookAndFeel->scale = resizeRatio;
        });
    resizeCorner->onFinish = [this]()
        {
            resizing = false;
            audioProcessor.scale = resizeRatio;
            audioProcessor.saveSettings();
        };

    oscA = std::make_unique<OSCPanel>(*this, 0);
    addAndMakeVisible(oscA.get());
    oscA->setBounds(PANEL_PAD, PANEL_PAD, KNOB_WIDTH * 4 + KNOB_WIDTH_SM * 3, KNOB_HEIGHT * 2 + PANEL_HEADER_HEIGHT + 7);

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

    envelopes = std::make_unique<EnvDisplay>(*this);
    addAndMakeVisible(envelopes.get());
    envelopes->setBounds(Rectangle<int>(oscA->getX() + 75, oscD->getBottom() + PANEL_PAD, oscA->getWidth() - 75, 10)
        .withBottom(getBottom() - PANEL_PAD));

    globals = std::make_unique<GlobalsPanel>(*this);
    addAndMakeVisible(globals.get());
    globals->setVisible(!audioProcessor.fmMatrixVisible);
    globals->setBounds(filter2->getBounds().withY(filter2->getBottom() + PANEL_PAD).withBottom(getHeight() - PANEL_PAD));

    fmMatrix = std::make_unique<FmMatrixPanel>(*this);
    addAndMakeVisible(fmMatrix.get());
    fmMatrix->setVisible(audioProcessor.fmMatrixVisible);
    fmMatrix->setBounds(globals->getBounds());


    tmp = std::make_unique<TMP>(audioProcessor);
    addAndMakeVisible(tmp.get());
    tmp->setBounds(100, 20, 100, 20);

    juce::Desktop::getInstance().addGlobalMouseListener(this);
    audioProcessor.modulation->UIDirty.store(true); // refresh connections on startup

    toggleFmMatrix(); // REMOVE ME
}

void TetraOPAudioProcessorEditor::toggleFmMatrix()
{
    audioProcessor.fmMatrixVisible = !audioProcessor.fmMatrixVisible;
    globals->setVisible(!audioProcessor.fmMatrixVisible);
    fmMatrix->setVisible(audioProcessor.fmMatrixVisible);
}

void TetraOPAudioProcessorEditor::rebuild()
{
    PopupMenu::dismissAllActiveMenus();
    modulatedParams.clear();
    juce::Desktop::getInstance().removeGlobalMouseListener(this);

    removeAllChildren();

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
    /*
    if (isDragDropModulation) {
        dragDropOverlay->repaint();
    }

    auto connections = audioProcessor.modulation->getConnections();
    auto voiceActive = audioProcessor.modulation->isAnyVoiceActive();

    // refresh modulation values for all connected params
    for (auto& conn : connections) {
        if (modulatedParams.find(conn.dst) != modulatedParams.end()) {
            auto param = modulatedParams[conn.dst];
            if (param->isShowing()) {
                auto val = audioProcessor.modulation->getModulatedNorm(conn.dst);
                if (param->modValue != val || param->voiceActive != voiceActive) { // save on repaints
                    param->modValue = val;
                    param->voiceActive = voiceActive;
                    param->repaint();
                }
            }
        }
    }

    // handle new modulation connects or disconnects
    // also handle selected modulator change
    if (audioProcessor.modulation->UIDirty.load()) {
        audioProcessor.modulation->UIDirty.store(false);

        if (matrixPanel->isVisible()) {
            matrixPanel->setConnections(connections);
        }

        std::unordered_set<std::string> modulated;
        for (auto& conn : connections) {
            modulated.insert(conn.dst);
        }

        // handle connects and disconnects
        for (auto& [paramId, param] : modulatedParams) {
            param->modulated = modulated.count(paramId) > 0;
            param->setModId(""); // start by disabling current modulator for every param
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
                    param->setModId(juce::String("mod") + juce::String(conn.id) + "_amt");
                    param->modBipolar = conn.bipolar;
                    param->modColor = conn.bypass ? Colours::transparentBlack : selmodColor;
                }
            }
        }

        // finally repaint all params
        for (auto& [paramId, param] : modulatedParams) {
            if (param->isShowing()) {
                param->repaint();
            }
        }
    }
    */
}

void TetraOPAudioProcessorEditor::startDragDrop(String modID, juce::Component* component)
{
    (void)modID;
    (void)component;
    /*
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
    auto centre = component->getLocalBounds().getCentre();
    dragDropOverlay->arrowStart = dragDropOverlay->getLocalPoint(component, centre).toFloat();
    dragDropOverlay->arrowEnd = dragDropOverlay->arrowStart;
    dragDropOverlay->arrowColor = modColor;

    auto connections = audioProcessor.modulation->getConnections();
    std::unordered_set<std::string> modulated;
    for (auto& conn : connections) {
        if (conn.src == modID.toStdString()) {
            modulated.insert(conn.dst);
        }
    }

    for (auto& [paramId, param] : modulatedParams) {
        if (param->isShowing() && modulated.count(paramId) == 0) {
            param->showDragAndDrop = true;
            param->dragAndDropColour = modColor.withAlpha(0.5f);
            param->repaint();
        }
    }
    */
}

/*
* On global mouse up finish drag and drop of modulators
*/
void TetraOPAudioProcessorEditor::mouseUp(const juce::MouseEvent& e)
{
    (void)e;
    /*
    if (isDragDropModulation) {
        isDragDropModulation = false;
        dragDropOverlay->setVisible(false);

        auto* comp = getComponentAt(e.getEventRelativeTo(this).getPosition());
        if (comp == nullptr)
            return;

        auto id = comp->getName();

        // for each modulatable param turn off drag and drop
        // if the drag and drop finished on the param make a new connection
        for (auto& [paramId, param] : modulatedParams) {
            if (param->showDragAndDrop) {
                param->showDragAndDrop = false;
                if (param->getName() == id) {
                    audioProcessor.modulation->connect(dragDropModID.toStdString(), id.toStdString());
                }
                param->repaint();
            }
        }

        audioProcessor.modulation->setSelectedMod(dragDropModID.toStdString());
        dragDropModID = "";
    }
    */
}

/*
* Shortcut to connect a param with current selected modulator on mouse down with control pressed
* Allows to quickly create connections without using drag and drop
*/
void TetraOPAudioProcessorEditor::quickConnect(String paramId)
{
    (void)paramId;
    /*
    if (!modulatedParams.count(paramId.toStdString()))
        return;

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
    param->modId = modSliderId;
    */
}

void TetraOPAudioProcessorEditor::setMouseHoverParam(ModulatedParam* param)
{
    mouseHoverParam = param;
}

void TetraOPAudioProcessorEditor::registerModParam(ModulatedParam* param)
{
    modulatedParams[param->paramId.toStdString()] = param;
}

void TetraOPAudioProcessorEditor::unregisterModParam(ModulatedParam* param)
{
    modulatedParams.erase(param->paramId.toStdString());
}

void TetraOPAudioProcessorEditor::selectTab(int tab)
{
    /*
    effectsPanel->setVisible(tab == 1);
    matrixPanel->setVisible(tab == 2);
    configsPanel->setVisible(tab == 3);
    browserPanel->setVisible(tab == 4);

    resAPanelL1->setVisible(tab == 0);
    resBPanelL1->setVisible(tab == 0);
    malletPanelL1->setVisible(tab == 0);
    noisePanelL1->setVisible(tab == 0);
    */
    audioProcessor.selectedTab = tab;

    /*
    header->repaint();

    if (matrixPanel->isVisible()) {
        audioProcessor.modulation->UIDirty.store(true); // trigger a connection refresh
    }

    // on browser tab move the modulators panel down so that the macros are visible
    modulatorsPanel->setBounds(modulatorsPanel->getBounds()
        .withY(couplePanelL1->getBottom() + PANEL_MARGIN + (tab == 4 ? 70 : 0)));
    */

    repaint();
}

void TetraOPAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    (void)parameterID;
    (void)newValue;
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

/*
void TetraOPAudioProcessorEditor::showMacroRename(Macro* macro)
{
    (void)macro;
    auto paramPos = getLocalPoint(macro, macro->getLocalBounds().getCentre());
    textInputOverlay = std::make_unique<TextInput>(paramPos.x, paramPos.y, [this, macro](String text)
        {
            if (text.isEmpty()) {
                text = String("Macro") + String(macro->index);
            }
            audioProcessor.modulation->macroNames[macro->index] = text;
            removeChildComponent(textInputOverlay.get());
        }, [this]()
            {
                removeChildComponent(textInputOverlay.get());
            });
        textInputOverlay->setBounds({ 0, 0, getWidth(), getHeight() });
        addAndMakeVisible(textInputOverlay.get());
        textInputOverlay->focus();
    }
*/
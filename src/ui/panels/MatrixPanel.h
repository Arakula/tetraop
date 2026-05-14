#pragma once

#include <JuceHeader.h>
#include <functional>
#include "../UIUtils.h"
#include "../widgets/HSlider.h"
#include "../../engine/Modulation.h"
#include "../../dsp/Pattern.h"
#include "../ModulatedParam.h"
#include "../widgets/PowerCurve.h"
#include "../widgets/MapEditor.h"

class TetraOPAudioProcessorEditor;

class MapPreview : public juce::Component
{
public:
    MapPreview(Modulation::Connection _conn, std::function<void(Modulation::Connection conn)> _onClick)
        : conn(_conn)
        , onClick(_onClick)
    {
        setConn(conn);
    }

    void setConn(Modulation::Connection _conn)
    {
        conn = _conn;
        enabled = _conn.mapped;
        Modulation::stringToMap(_conn.mpoints, pat);
        repaint();
    }

    void mouseDown(const MouseEvent& e)
    {
        (void)e;
        onClick(conn);
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced(1.5f);
        UIUtils::drawBevel(g, b.reduced(1.f), 4.f, COLOR_BEVEL().withAlpha(!enabled ? 0.5f : 1.f));
        b = b.reduced(4.f);

        Path p;
        p.startNewSubPath(b.getX(), b.getY() + b.getHeight() * (1 - pat.get_y_at(0.f)));
        auto w = b.getWidth();
        for (int i = 0; i < w; ++i) {
            auto x = i / (float)w;
            auto y = 1.f - pat.get_y_at(x);
            p.lineTo(b.getX() + i, b.getY() + b.getHeight() * y);
        }
        g.setColour(COLOR_ACTIVE().withAlpha(!enabled ? 0.5f : 1.f));
        g.strokePath(p, PathStrokeType(1.f));
        g.setOpacity(1.f);
    }

private:
    Modulation::Connection conn;
    std::function<void(Modulation::Connection conn)> onClick;
    bool enabled = false;
    Pattern pat;
};

class Row : public juce::Component
{
public:
    Row(TetraOPAudioProcessorEditor& e
        , Modulation::Connection conn
        , Rectangle<float> bid
        , Rectangle<float> bsrc
        , Rectangle<float> bmap
        , Rectangle<float> bcurve
        , Rectangle<float>bpolar
        , Rectangle<float>bamount
        , Rectangle<float>bdst
        , Rectangle<float>bpass
        , std::function<void(Modulation::Connection conn)> onClickMap
    );
    ~Row() override {}

    bool isKeyTrackPitch(Modulation::Connection c);
    void setConnection(Modulation::Connection _conn);
    void paint(Graphics& g) override;
    void resized() override;
    void disconnect();

    TextButton src;
    std::unique_ptr<PowerCurve> curve;
    TextButton polar;
    std::unique_ptr<HSlider> amount;
    TextButton dst;
    TextButton bypass;
    std::unique_ptr<MapPreview> map;

    bool isEditingMap = false;
    Modulation::Connection conn;

private:
    std::function<void(Modulation::Connection conn)> onClickMap;
    TetraOPAudioProcessorEditor& editor;
    Rectangle<float> bid;
    Rectangle<float> bsrc;
    Rectangle<float> bmap;
    Rectangle<float> bpow;
    Rectangle<float> bpolar;
    Rectangle<float> bamount;
    Rectangle<float> bdst;
    Rectangle<float> bpass;
};

class MatrixPanel
    : public juce::Component
    , private juce::AudioProcessorValueTreeState::Listener
{
public:

    MatrixPanel(TetraOPAudioProcessorEditor& e);
    ~MatrixPanel() override;

    void setConnections(std::vector<Modulation::Connection> _conns);

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void paint(Graphics& g) override;
    void resized() override;
    void toggleUIComponents();
    void onEditMapChange();

private:
    std::vector<Modulation::Connection> conns{};
    TetraOPAudioProcessorEditor& editor;
    float margin = 5.f;
    Rectangle<float> bid;
    Rectangle<float> bsrc;
    Rectangle<float> bmap;
    Rectangle<float> bpow;
    Rectangle<float> bpolar;
    Rectangle<float> bamount;
    Rectangle<float> bdst;
    Rectangle<float> bpass;

    TextButton newBtn;
    std::unique_ptr<MapEditor> mapEditor;

    juce::Viewport list;
    juce::Component content;
    juce::OwnedArray<Row> rows;
};
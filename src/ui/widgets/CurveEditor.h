#pragma once

#include <JuceHeader.h>
#include "../widgets/Rotary.h"
#include "../../dsp/Pattern.h"
#include "../UIUtils.h"
#include "Multiselect.h"
#include <functional>
#include "../../Globals.h"

using namespace globals;

class TetraOPAudioProcessorEditor;

class CurveEditor : public juce::Component
{
public:
    CurveEditor(TetraOPAudioProcessorEditor& e, Pattern* p, float pad, juce::Colour c, juce::Colour selc, bool islfo, bool use_multiselect);
    ~CurveEditor() override;

    void setPattern(Pattern* p);

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void paint(juce::Graphics& g) override;
    void drawPreSelection(juce::Graphics& g);
    void drawGrid(juce::Graphics& g) const;
    void resized() override;

    void clearSelection();
    void insertNewPoint(const juce::MouseEvent& e);
    uint64_t getHoveredPoint(int x, int y);
    uint64_t getHoveredMidpoint(int x, int y);
    Pattern::PPoint& getPoint(uint64_t id);
    int getPointIndex(uint64_t id);
    std::vector<double> getMidpointXY(Pattern::Segment seg);
    Pattern::PPoint& getPointFromSegmentIndex(int midpoint);
    bool isSnapping(const juce::MouseEvent& e) const;
    bool isCollinear(Pattern::Segment seg);
    bool pointInRect(int x, int y, int xx, int yy, int w, int h);

    float mpoint_radius = 0.f;
    float point_radius = 0.f;
    float point_hover = 0.f;
    int gridX = 16;
    int gridY = 16;
    bool snap = false;
    bool drawSeek = false;
    bool drawShade = true;
    bool drawBasicSeek = false;
    bool useGrid = true;
    float seekPos = 0.f;
    std::function<void()>onChange;
    std::function<void(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)>onWheel;
private:
    juce::Point<int> preSelectionStart{};
    juce::Point<int> preSelectionEnd{};
    bool usemultisel = false;
    bool islfo = false;
    juce::Colour color;
    float pad;
    Multiselect multisel;
    TetraOPAudioProcessorEditor& editor;
    juce::Rectangle<float> viewBounds;
    float winw = 0.f;
    float winh = 0.f;
    float winx = 0.f;
    float winy = 0.f;
    int dragStartY = 0;
    uint64_t hoverPoint = 0;
    uint64_t selectedPoint = 0;
    uint64_t selectedMidpoint = 0;
    uint64_t hoverMidpoint = 0;
    float origTension = 0.f;
    Pattern::PPoint dummyPoint{ 0, 0.f, 0.f, 0.f, 1, 0 };
    Pattern* pattern;
    float wheelAccum = 0.f;
};
/*
  ==============================================================================

    Multiselect.h
    Author:  tiagolr

  ==============================================================================
*/
#pragma once

#include <JuceHeader.h>
#include <iostream>
#include "../../Globals.h"
#include "../../dsp/Pattern.h"

class TetraOPAudioProcessor;

class Multiselect
{
public:
	struct SelPoint {
	    uint64_t id;
	    double x; // 0..1 in relation to viewport
	    double y;
	    double areax; // 0..1 in relation to selection area
	    double areay;
	};

	enum MouseHover {
	    area,
	    topLeft,
	    topMid,
	    topRight,
	    midLeft,
	    midRight,
	    bottomLeft,
	    bottomMid,
	    bottomRight
	};

	struct Vec2 {
	    double x, y;

	    Vec2() : x(0), y(0) {}
	    Vec2(double x_, double y_) : x(x_), y(y_) {}
	    juce::Point<int> toPoint() { return juce::Point<int>((int)x , (int)y); }

	    Vec2 operator*(double scalar) const { return Vec2(x * scalar, y * scalar); }
	    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
	};

	using Quad = std::array<Vec2, 4>;

	inline Vec2 bilinearInterpolate(const Quad& q, double u, double v) {
	    Vec2 A = q[0] * (1 - u) + q[1] * u;  // Top edge interpolation
	    Vec2 B = q[2] * (1 - u) + q[3] * u;  // Bottom edge interpolation
	    Vec2 P = A * (1 - v) + B * v;      // Final vertical interpolation
	    return P;
	}

	Multiselect(TetraOPAudioProcessor& p);

	~Multiselect() {}

	void setPattern(Pattern* p);
    void setViewBounds(int _x, int _y, int _w, int _h);

	void mouseDown(const juce::MouseEvent& e);
    void mouseMove(const juce::MouseEvent& e);
    void mouseDrag(const juce::MouseEvent& e, int gridX, int gridY);
    void mouseUp(const juce::MouseEvent& e);
	void drawBackground(juce::Graphics& g);
	void draw(juce::Graphics& g);
    void drawHandles(juce::Graphics& g);

    bool isSelected(uint64_t id);
    void recalcSelectionArea();
    void clearSelection();
    void makeSelection(const juce::MouseEvent& e, juce::Point<int>selectionStart, juce::Point<int>selectionEnd);
    void deleteSelectedPoints();
    void selectAll();

    int mouseHover = -1; // flag for hovering selection drag handles, 0 area, 1 top left corner, 2 top center etc..
    std::vector<SelPoint> selectionPoints;
    bool dragging = false;
	juce::Colour color;

private:
	Pattern* pattern = nullptr;
    int winx = 0;
    int winy = 0;
    int winw = 0;
    int winh = 0;
    // select quad coordinates
    Quad quad = { Vec2(0.0,0.0),Vec2(1.0,0.0),Vec2(0.0,1.0),Vec2(1.0,1.0) };
    // select quad relative coordinates to selection area
    Quad quadrel = { Vec2(0.0, 0.0), Vec2(0.0, 0.0), Vec2(0.0, 0.0), Vec2(0.0, 0.0) };
    bool invertx = false;
    bool inverty = false;

    void dragArea(const juce::MouseEvent& e, int gridX, int gridY);
    void dragQuad(const juce::MouseEvent& e, int gridX, int gridY);
    void updatePointsToSelection();

    Quad getQuadExpanded(double expand = 0.0);
    void calcRelativeQuadCoords(juce::Rectangle<double> area);
    void applyRelativeQuadCoords(juce::Rectangle<double> area);

	juce::Rectangle<double> selectionAreaStart = juce::Rectangle<double>(); // used to drag or scale selection area
    Quad selectionQuadStart = {Vec2(0.0,0.0), Vec2(1.0, 0.0), Vec2(0.0, 1.0), Vec2(1.0,1.0)};
    TetraOPAudioProcessor& audioProcessor;
    Theme& theme;

    bool isSnapping(const juce::MouseEvent& e);
    Vec2 pointToVec(juce::Point<double> p);
    juce::Rectangle<double> quadToRect(Quad q);
    bool isCollinear(const std::vector<SelPoint>& p, bool xaxis);
};

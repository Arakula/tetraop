/*
  ==============================================================================

    Pattern.h
    Author:  tiagolr

  ==============================================================================
*/

#pragma once
#include <vector>
#include <mutex>
#include <atomic>

class Pattern
{
	public:
	    enum PointType {
	    Hold,
	    Curve,
	    SCurve,
	    Pulse,
	    Wave,
	    Triangle,
	    Stairs,
	    SmoothSt,
        SinCurve,
	};

	enum PointRole {
	    StdPoint,
	    DelPoint,
	    AttPoint,
	    DecPoint,
	    SusPoint,
	    RelPoint,
	};

	struct PPoint {
	    uint64_t id; // unique point id
	    float x;
	    float y;
	    float tension;
	    int type;
	    int role;
	};

	struct Segment {
	    float x1;
	    float x2;
	    float y1;
	    float y2;
	    float tension;
	    float power;
	    int type;
	};

    uint64_t versionID = 0; // unique pattern ID, used by UI to detect pattern changes
    static std::vector<PPoint> copy_pattern;
    static constexpr float PI = 3.14159265358979323846f;
    int index;
    std::vector<PPoint> points;
    std::vector<Segment> segments;
    std::vector<std::vector<PPoint>> undoStack;
    std::vector<std::vector<PPoint>> redoStack;
    std::atomic<float> tensionMult = 0.0; // tension multiplier applied to all points

    Pattern();
    Pattern(int index);
    void incrementVersion(); // generates a new unique ID for this pattern

    int insertPoint(float x, float y, float tension, int type, int role = 0);
    void sortPoints();
    void setTension(float t); // sets global tension multiplier
    void removePoint(float x, float y);
    void removePoint(int i);
    void removePointsInRange(float x1, float x2);
    void invert();
    void reverse();
    void floatPattern();
    void rotate(float x);
    void clear();
    void buildSegments();
    void loadSine();
    void loadTriangle();
    void copyFrom(Pattern& p);
    void copy();
    void paste();
    std::vector<Segment> getSegments();
    int getWaveCount(Segment seg);

    float get_y_curve(Segment seg, float x);
    float get_y_scurve(Segment seg, float x);
    float get_y_pulse(Segment seg, float x);
    float get_y_wave(Segment seg, float x);
    float get_y_triangle(Segment seg, float x);
    float get_y_stairs(Segment seg, float x);
    float get_y_smooth_stairs(Segment seg, float x);
    float get_y_sincurve(Segment seg, float x);
    float get_y_at(float x);

    void createUndo();
    void undo();
    void redo();
    void clearUndo();
    static bool comparePoints(const std::vector<PPoint>& a, const std::vector<PPoint>& b);

    std::string serialize();
    void unserialize(std::string pts);

private:
    static inline uint64_t versionIDCounter = 1; // static global ID counter
    static inline uint64_t pointsIDCounter = 1; // static global ID counter
    bool dualTension = false;
    std::mutex mtx;
    std::mutex pointsmtx;
};
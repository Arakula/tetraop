#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;

class UIUtils {
public:
	// Mouse wheel sensitivity for proportional value-editing widgets.
	// slider_change = wheel.deltaY * (shift ? WHEEL_SPEED_FINE : WHEEL_SPEED)
	static constexpr float WHEEL_SPEED      = 0.2f;
	static constexpr float WHEEL_SPEED_FINE = 0.04f;
	// Accumulator threshold for stepped widgets (one step ~= one mouse-wheel notch worth of deltaY).
	static constexpr float WHEEL_STEP       = 0.2f;
    // Converts a wheel event into integer steps for stepped widgets.
	// For stepped wheels (isSmooth=false), each event is exactly +/-1 step.
	// For smooth devices (trackpads), deltaY accumulates into `accum` and a step
	// is emitted whenever |accum| crosses WHEEL_STEP. Honours wheel.isReversed.
	static int wheelStep(const juce::MouseWheelDetails& wheel, float& accum);

	// Returns the normalized change to apply to `param` for this wheel event.
	// For continuous params (sub-1% interval), returns proportional `deltaY * speed`.
	// For stepped params, accumulates deltaY into `accum` and emits +/- one
	// normalized interval per WHEEL_STEP crossed (or per stepped-wheel notch).
	// Honours wheel.isReversed.
	static float wheelChange(const juce::MouseWheelDetails& wheel,
	                         juce::RangedAudioParameter* param,
	                         float& accum,
	                         float speed);

	static juce::String capitalize(juce::String str);
	static juce::String aliasModulator(const juce::String& input);
    static juce::String aliasParameter(const juce::String& pid);

	static void drawBevel(Graphics& g, Rectangle<float> bounds, float corner, Colour bg);
	static void drawTriangle(Graphics& g, Rectangle<float> bounds, int direction, Colour c);
	static void drawPanel(Graphics& g, Rectangle<float> bounds, bool drawHeader, bool darker = false, Colour headerc = Colours::transparentBlack);
	static void drawVSeparator(Graphics& g, Rectangle<float> bounds);
	static void drawCheckmark(Graphics& g, Rectangle<float> bounds, Colour bg, Colour check, bool checked);
	static void drawFeedback(Graphics& g, Rectangle<float> bounds, Colour c);
	static void drawHandle(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c);

	static void drawRand(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c);
	static void drawRandGen(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c);
	static void drawHand(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c, juce::Point<int> tx = { 0,0 });
	static void drawKeys(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c);
	static void drawVel(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c);
	static void drawModwheel(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c);
	static void drawXMod(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c);
	static void drawYMod(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c);
	static void drawZMod(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c);
	static void drawLift(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c);
	static void drawGear(juce::Graphics& g, juce::Rectangle<int> bounds, float radius, int segs, juce::Colour color, juce::Colour background);
	static void drawClose(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c, float lineWidth);
	static void drawEdit(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c, float scale);
	static void drawSineWave(juce::Graphics& g, juce::Rectangle<float> bounds, int n, juce::Colour c);

	static void drawPowerButton(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c, float scale);
	static void drawWheel(juce::Graphics& g, juce::Rectangle<float> bounds, float wheelnorm);
	static void drawUndo(juce::Graphics& g, juce::Rectangle<float> area, bool invertx, juce::Colour color);
	static void drawClock(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color);
	static void drawNote(juce::Graphics& g, juce::Rectangle<float> bounds, int mode, juce::Colour color);
	static void drawChain(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color);

	static void drawLogo(juce::Graphics& g, Rectangle<float> bounds, Colour color);
	static void drawSave(juce::Graphics& g, Rectangle<float> bounds, Colour color);
	static void drawEllipsis(juce::Graphics& g, Rectangle<float> bounds, Colour color);
	static void drawCPU(juce::Graphics& g, Rectangle<float> bounds, Colour color);
	static void drawVoices(juce::Graphics& g, Rectangle<float> bounds, Colour color);

    static bool startUnboundedMouse(juce::Component& c, const juce::MouseEvent& e, bool unbounded)
    {
        if (!unbounded)
            return false;

        c.setMouseCursor(juce::MouseCursor::NoCursor);
        e.source.enableUnboundedMouseMovement(true);

        return true;
    }

    static bool stopUnboundedMouse(juce::Component& c, const juce::MouseEvent& e, bool unbounded)
    {
        if (!unbounded)
            return false;

        c.setMouseCursor(juce::MouseCursor::NormalCursor);
        e.source.enableUnboundedMouseMovement(false);

        return true;
    }
};
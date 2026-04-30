#pragma once

#include <JuceHeader.h>
#include "../Globals.h"

using namespace globals;
class RipplerAudioProcessorEditor;

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

	static void drawBevel(Graphics& g, Rectangle<float> bounds, float corner, Colour bg);
	static void drawTriangle(Graphics& g, Rectangle<float> bounds, int direction, Colour c);
	static void drawPanel(Graphics& g, Rectangle<float> bounds, bool drawHeader);
	static void drawVSeparator(Graphics& g, Rectangle<float> bounds);
	static void drawCheckmark(Graphics& g, Rectangle<float> bounds, Colour bg, Colour check, bool checked);
	static void drawFeedback(Graphics& g, Rectangle<float> bounds, Colour c);

    static bool startUnboundedMouse(juce::Component& c, const juce::MouseEvent& e)
    {
        //juce::SharedResourcePointer<Preferences> prefs;
        //if (!prefs->unboundedMouse)
        //    return false;

        c.setMouseCursor(juce::MouseCursor::NoCursor);
        e.source.enableUnboundedMouseMovement(true);

        return true;
    }

    static bool stopUnboundedMouse(juce::Component& c, const juce::MouseEvent& e)
    {
        //juce::SharedResourcePointer<Preferences> prefs;
        //if (!prefs->unboundedMouse)
        //    return false;

        c.setMouseCursor(juce::MouseCursor::NormalCursor);
        e.source.enableUnboundedMouseMovement(false);

        return true;
    }
};
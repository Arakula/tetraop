#include "UIUtils.h"

int UIUtils::wheelStep(const juce::MouseWheelDetails& wheel, float& accum)
{
    auto deltaY = wheel.isReversed ? -wheel.deltaY : wheel.deltaY;
    if (deltaY == 0.f) return 0;

    if (!wheel.isSmooth) {
        // stepped device (mouse wheel): one notch = one step
        accum = 0.f;
        return deltaY > 0.f ? 1 : -1;
    }

    // smooth device (trackpad): accumulate and emit a step per WHEEL_STEP crossed
    accum += deltaY;
    int steps = 0;
    while (accum >= WHEEL_STEP)  { accum -= WHEEL_STEP; ++steps; }
    while (accum <= -WHEEL_STEP) { accum += WHEEL_STEP; --steps; }
    return steps;
}

float UIUtils::wheelChange(const juce::MouseWheelDetails& wheel,
                            juce::RangedAudioParameter* param,
                            float& accum,
                            float speed)
{
    auto deltaY = wheel.isReversed ? -wheel.deltaY : wheel.deltaY;
    if (deltaY == 0.f) return 0.f;

    // determine how granular this param is in normalized 0..1 space
    float normInterval = 0.f;
    if (param != nullptr) {
        auto range = param->getNormalisableRange();
        if (range.interval > 0.f && range.end > range.start)
            normInterval = range.interval / (range.end - range.start);
    }

    // continuous (or essentially continuous) param: proportional scaling
    // 0.01 corresponds to 100 distinct values - finer than that feels continuous
    if (normInterval < 0.01f) {
        accum = 0.f;
        return deltaY * speed;
    }

    // stepped param + stepped wheel: one notch = one interval
    if (!wheel.isSmooth) {
        accum = 0.f;
        return deltaY > 0.f ? normInterval : -normInterval;
    }

    // stepped param + smooth device: accumulate, emit one interval per WHEEL_STEP
    accum += deltaY;
    int steps = 0;
    while (accum >= WHEEL_STEP)  { accum -= WHEEL_STEP; ++steps; }
    while (accum <= -WHEEL_STEP) { accum += WHEEL_STEP; --steps; }
    return (float)steps * normInterval;
}

void UIUtils::drawBevel(Graphics& g, Rectangle<float> bounds, float corner, Colour bg)
{
    bounds = bounds.translated(0.5f, 0.5f);
    juce::ColourGradient gradient(
        Colour(0xff0F0F0F).withAlpha(0.35f), bounds.getX(), bounds.getY(),
        Colours::white.withAlpha(0.12f), bounds.getX(), bounds.getBottom(), false
    );
    gradient.addColour(0.5, Colour(0xff0F0F0F).withAlpha(0.02f));
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, corner);

    g.setColour(bg);
    g.fillRoundedRectangle(bounds.expanded(-1.f), corner);
}

void UIUtils::drawTriangle(Graphics& g, Rectangle<float> bounds, int direction, Colour c)
{
    g.setColour(c);
    Path p;
    if (direction == 0) { // top
        p.startNewSubPath(bounds.getBottomLeft());
        p.lineTo({ bounds.getCentreX(), bounds.getY() });
        p.lineTo(bounds.getBottomRight());
    }
    else if (direction == 1) { // right
        p.startNewSubPath(bounds.getTopLeft());
        p.lineTo({ bounds.getRight(), bounds.getCentreY() });
        p.lineTo(bounds.getBottomLeft());
    }
    else if (direction == 2) { // bottom
        p.startNewSubPath(bounds.getTopLeft());
        p.lineTo({ bounds.getCentreX(), bounds.getBottom() });
        p.lineTo(bounds.getTopRight());
    }
    else { // right
        p.startNewSubPath(bounds.getTopRight());
        p.lineTo({ bounds.getX(), bounds.getCentreY() });
        p.lineTo(bounds.getBottomRight());
    }
    p.closeSubPath();
    g.fillPath(p);
}

void UIUtils::drawPanel(Graphics& g, Rectangle<float> bounds, bool drawHeader)
{
    bounds = bounds.translated(0.f, 0.f);
	g.setColour(COLOR_PANEL());
	g.fillRoundedRectangle(bounds, PANEL_CORNER);

    g.setColour(Colours::white.withAlpha(0.1f));
    g.drawRoundedRectangle(bounds, PANEL_CORNER, 1.f);

    if (drawHeader) {
        Path p;
        p.addRoundedRectangle(bounds.getX() + 1, bounds.getY() + 1,
            bounds.getWidth() - 2, PANEL_HEADER_HEIGHT - 1,
            PANEL_CORNER, PANEL_CORNER,
            true, true, false, false
        );
        g.setColour(COLOR_PANEL_HEADER());
        g.fillPath(p);

        g.setColour(Colours::white.withAlpha(0.6f));
        g.strokePath(p, PathStrokeType(1.f));

        // draw header stroke
        g.setColour(Colours::black.withAlpha(0.35f));
        g.drawHorizontalLine((int)(bounds.getY() + PANEL_HEADER_HEIGHT + 1), bounds.getX(), bounds.getRight());
    }
}

void UIUtils::drawVSeparator(Graphics& g, Rectangle<float> bounds)
{
    auto x = bounds.getCentreX();
    g.setColour(Colours::black.withAlpha(0.25f));
    g.drawVerticalLine((int)x, bounds.getY(), bounds.getBottom());
    g.setColour(Colours::white.withAlpha(0.25f));
    g.drawVerticalLine((int)x + 1, bounds.getY(), bounds.getBottom());
}

void UIUtils::drawCheckmark(Graphics& g, Rectangle<float> bounds, Colour bg, Colour check, bool checked)
{
    auto b = Rectangle<float>(bounds.getCentreX() - 5, bounds.getCentreY() - 5, 10.f, 10.f).translated(0.5f, 0.5f);
    g.setColour(bg);
    g.fillRoundedRectangle(b, 1.f);
    if (checked)
    {
        g.setColour(check.withAlpha(0.5f));
        g.fillRect(b.reduced(1.f));
        g.setColour(check);
        g.fillRect(b.reduced(2.f));
    }
}

void UIUtils::drawFeedback(Graphics& g, Rectangle<float> b, Colour c)
{
    b.reduce(0.5f, 3.5f);
    Path p;
    p.startNewSubPath(b.getTopLeft().translated(3, 0));
    p.lineTo(b.getTopRight());
    p.lineTo(b.getBottomRight());
    p.lineTo(b.getBottomLeft().translated(3, 0));

    g.setColour(c);
    g.strokePath(p, PathStrokeType(1.f));
    UIUtils::drawTriangle(g, b.withWidth(6.f).withHeight(6.f).withBottomY(b.getBottom()).translated(0, 3.f), 3, c);
}
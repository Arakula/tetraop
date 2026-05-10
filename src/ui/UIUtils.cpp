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

void UIUtils::drawPanel(Graphics& g, Rectangle<float> bounds, bool drawHeader, bool darker)
{
    bounds = bounds.translated(0.f, 0.f);
	g.setColour(COLOR_PANEL().darker(darker ? 0.6f : 0.f));
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

void UIUtils::drawHandle(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;

    p.startNewSubPath(3.0f, 5.0f);
    p.lineTo(1.0f, 7.0f);
    p.lineTo(3.0f, 9.0f);

    p.startNewSubPath(11.0f, 5.0f);
    p.lineTo(13.0f, 7.0f);
    p.lineTo(11.0f, 9.0f);

    p.startNewSubPath(5.0f, 11.0f);
    p.lineTo(7.0f, 13.0f);
    p.lineTo(9.0f, 11.0f);

    p.startNewSubPath(5.0f, 3.0f);
    p.lineTo(7.0f, 1.0f);
    p.lineTo(9.0f, 3.0f);

    p.startNewSubPath(1.5f, 7.0f);
    p.lineTo(4.0f, 7.0f);

    p.startNewSubPath(10.0f, 7.0f);
    p.lineTo(12.5f, 7.0f);

    p.startNewSubPath(7.0f, 2.0f);
    p.lineTo(7.0f, 4.0f);

    p.startNewSubPath(7.0f, 10.0f);
    p.lineTo(7.0f, 12.0f);

    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}


void UIUtils::drawRand(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;

    p.addRectangle(11.0f, 2.0f, 2.0f, 2.0f);
    p.addRectangle(16.0f, 0.0f, 2.0f, 2.0f);
    p.addRectangle(6.0f, 0.0f, 2.0f, 2.0f);
    p.addRectangle(0.0f, 4.0f, 2.0f, 2.0f);
    p.addRectangle(5.0f, 7.0f, 2.0f, 2.0f);
    p.addRectangle(26.0f, 7.0f, 2.0f, 2.0f);
    p.addRectangle(20.0f, 4.0f, 2.0f, 2.0f);
    p.addRectangle(24.0f, 0.0f, 2.0f, 2.0f);
    p.addRectangle(14.0f, 8.0f, 2.0f, 2.0f);

    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.fillPath(p);
}

void UIUtils::drawRandGen(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;
    p.startNewSubPath(0.0f, 6.0f);
    p.lineTo(3.0f, 6.0f);
    p.lineTo(3.0f, 12.0f);
    p.lineTo(8.0f, 12.0f);
    p.lineTo(8.0f, 2.0f);
    p.lineTo(11.0f, 2.0f);
    p.lineTo(11.0f, 8.0f);
    p.lineTo(16.0f, 8.0f);
    p.lineTo(16.0f, 3.0f);
    p.lineTo(22.0f, 3.0f);
    p.lineTo(22.0f, 10.0f);
    p.lineTo(26.0f, 10.0f);
    p.lineTo(26.0f, 0.0f);
    p.lineTo(32.0f, 0.0f);
    p.lineTo(32.0f, 6.0f);
    p.lineTo(38.0f, 6.0f);

    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
}

void UIUtils::drawHand(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c, juce::Point<int> tx)
{
    juce::Path p;

    p.startNewSubPath(1.0f, 13.5f);
    p.lineTo(1.0f, 8.0f);
    p.cubicTo(1.0f, 8.0f, 1.0f, 6.5f, 2.5f, 6.5f);
    p.cubicTo(4.0f, 6.5f, 4.0f, 8.0f, 4.0f, 8.0f);
    p.lineTo(4.0f, 8.5f);

    // === juce::Path 2 ===
    p.startNewSubPath(4.0f, 8.5f);
    p.lineTo(4.0f, 7.0f);
    p.cubicTo(4.0f, 7.0f, 4.0f, 5.5f, 5.5f, 5.5f);
    p.cubicTo(7.0f, 5.5f, 7.0f, 7.0f, 7.0f, 7.0f);
    p.lineTo(7.0f, 8.0f);

    // === juce::Path 3 ===
    p.startNewSubPath(7.0f, 8.5f);
    p.lineTo(7.0f, 6.0f);
    p.cubicTo(7.0f, 6.0f, 7.0f, 4.5f, 8.5f, 4.5f);
    p.cubicTo(10.0f, 4.5f, 10.0f, 6.0f, 10.0f, 6.0f);
    p.lineTo(10.0f, 7.0f);

    // === juce::Path 4 ===
    p.startNewSubPath(10.0f, 9.0f);
    p.lineTo(10.0f, 6.0f);
    p.lineTo(10.0f, 2.0f);
    p.cubicTo(10.0f, 2.0f, 10.0f, 0.5f, 11.5f, 0.5f);
    p.cubicTo(13.0f, 0.5f, 13.0f, 2.0f, 13.0f, 2.0f);
    p.lineTo(13.0f, 12.0f);
    p.lineTo(16.0f, 10.0f);
    p.cubicTo(16.3333f, 9.83333f, 17.0f, 9.5f, 17.5f, 10.0f);
    p.cubicTo(18.0f, 10.5f, 18.0f, 10.5f, 18.0f, 11.0f);
    p.cubicTo(18.0f, 11.5f, 16.0f, 13.0f, 16.0f, 13.0f);

    p.applyTransform(juce::AffineTransform::translation(bounds.getX() + tx.getX(), bounds.getY() + tx.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f));
}

void UIUtils::drawKeys(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;

    // Vertical lines
    p.startNewSubPath(12.5f, 8.0f);
    p.lineTo(12.5f, 12.0f);

    p.startNewSubPath(6.5f, 8.0f);
    p.lineTo(6.5f, 12.0f);

    p.addRoundedRectangle(4.5f, 0.5f, 3.0f, 8.0f, 1.0f, 1.0f, false, false, true, true);

    p.addRoundedRectangle(10.5f, 0.5f, 3.0f, 8.0f, 1.0f, 1.0f, false, false, true, true);

    p.addRoundedRectangle(0.5f, 0.5f, 17.0f, 12.0f, 2.f);

    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f));
}

void UIUtils::drawVel(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;

    p.startNewSubPath(3.0f, 13.0f);
    p.lineTo(3.0f, 9.0f);

    p.startNewSubPath(9.0f, 13.0f);
    p.lineTo(9.0f, 6.0f);

    p.startNewSubPath(15.0f, 13.0f);
    p.lineTo(15.0f, 3.0f);

    p.addRectangle(1.0f, 7.0f, 4.0f, 2.0f);
    p.addRectangle(7.0f, 4.0f, 4.0f, 2.0f);
    p.addRectangle(13.0f, 1.0f, 4.0f, 2.0f);

    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f));
}

void UIUtils::drawModwheel(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;

    juce::Path outer;
    outer.addCentredArc(9.5f, 9.5f, 8.5f, 8.5f, 0.0f,
        -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
    outer.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));

    juce::Path middle;
    middle.addCentredArc(9.5f, 9.5f, 7.5f, 7.5f, 0.0f,
        -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
    middle.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));

    juce::Path inner;
    inner.addCentredArc(9.5f, 9.5f, 4.5f, 4.5f, 0.0f,
        -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
    inner.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));

    juce::Path dashedPath;
    juce::PathStrokeType stroke(1.0f);
    float dashLengths[] = { 3.0f, 3.0f };
    stroke.createDashedStroke(dashedPath, outer, dashLengths, 2);
    g.setColour(c);
    g.strokePath(dashedPath, juce::PathStrokeType(1.0f));
    g.strokePath(middle, juce::PathStrokeType(1.0f));
    g.strokePath(inner, juce::PathStrokeType(1.0f));
}

void UIUtils::drawXMod(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;
    p.startNewSubPath(13.9434f, 3.00012f);
    p.lineTo(22.9434f, 3.00012f);
    p.startNewSubPath(22.9434f, 3.00012f);
    p.lineTo(18.9434f, 0.500122f);
    p.startNewSubPath(22.9434f, 3.00012f);
    p.lineTo(18.9434f, 5.50012f);

    p.startNewSubPath(9.94336f, 3.00012f);
    p.lineTo(0.943359f, 3.00012f);
    p.startNewSubPath(0.943359f, 3.00012f);
    p.lineTo(4.94336f, 0.500122f);
    p.startNewSubPath(0.943359f, 3.00012f);
    p.lineTo(4.94336f, 5.50012f);

    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void UIUtils::drawYMod(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;
    p.startNewSubPath(3.00012f, 10.9434f);
    p.lineTo(3.00012f, 0.943359f);
    p.startNewSubPath(3.00012f, 0.943359f);
    p.lineTo(0.500122f, 4.94336f);
    p.startNewSubPath(3.00012f, 0.943359f);
    p.lineTo(5.50012f, 4.94336f);

    p.startNewSubPath(11.0001f, 0.943359f);
    p.lineTo(11.0001f, 10.9434f);
    p.startNewSubPath(11.0001f, 10.9434f);
    p.lineTo(8.50012f, 6.94336f);
    p.startNewSubPath(11.0001f, 10.9434f);
    p.lineTo(13.5001f, 6.94336f);

    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void UIUtils::drawZMod(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;
    p.startNewSubPath(0.5f, 3.00012f);
    p.lineTo(9.5f, 3.00012f);
    p.startNewSubPath(9.5f, 3.00012f);
    p.lineTo(5.5f, 0.500122f);
    p.startNewSubPath(9.5f, 3.00012f);
    p.lineTo(5.5f, 5.50012f);

    p.startNewSubPath(29.5f, 3.00012f);
    p.lineTo(20.5f, 3.00012f);
    p.startNewSubPath(20.5f, 3.00012f);
    p.lineTo(24.5f, 0.500122f);
    p.startNewSubPath(20.5f, 3.00012f);
    p.lineTo(24.5f, 5.50012f);

    p.addEllipse(13.f, 1.f, 4.f, 4.f);

    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void UIUtils::drawLift(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;
    p.startNewSubPath(3.f, 10.9434f);
    p.lineTo(3.f, 0.943359f);
    p.startNewSubPath(3.f, 0.943359f);
    p.lineTo(0.5f, 4.94336f);
    p.startNewSubPath(3.f, 0.943359f);
    p.lineTo(5.5f, 4.94336f);

    p.startNewSubPath(11.f, 10.9434f);
    p.lineTo(11.f, 0.943359f);
    p.startNewSubPath(11.f, 0.943359f);
    p.lineTo(8.5f, 4.94336f);
    p.startNewSubPath(11.f, 0.943359f);
    p.lineTo(13.5f, 4.94336f);

    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void UIUtils::drawGear(juce::Graphics& g, juce::Rectangle<int> bounds, float radius, int segs, juce::Colour color, juce::Colour background)
{
    float x = bounds.toFloat().getCentreX();
    float y = bounds.toFloat().getCentreY();
    float oradius = radius;
    float iradius = radius / 3.f;
    float cradius = iradius / 1.5f;
    float coffset = juce::MathConstants<float>::twoPi;
    float inc = juce::MathConstants<float>::twoPi / segs;

    g.setColour(color);
    g.fillEllipse(x - oradius, y - oradius, oradius * 2.f, oradius * 2.f);

    g.setColour(background);
    for (int i = 0; i < segs; i++) {
        float angle = coffset + i * inc;
        float cx = x + std::cos(angle) * oradius;
        float cy = y + std::sin(angle) * oradius;
        g.fillEllipse(cx - cradius, cy - cradius, cradius * 2, cradius * 2);
    }
    g.fillEllipse(x - iradius, y - iradius, iradius * 2.f, iradius * 2.f);
}

void UIUtils::drawClose(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c, float lineWidth)
{
    juce::Path p;
    p.startNewSubPath(bounds.getBottomLeft());
    p.lineTo(bounds.getTopRight());
    p.startNewSubPath(bounds.getTopLeft());
    p.lineTo(bounds.getBottomRight());
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(lineWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void UIUtils::drawEdit(juce::Graphics& g, juce::Rectangle<float>bounds, juce::Colour c, float scale)
{
    juce::Path p;

    p.startNewSubPath(2.0f, 16.0f);
    p.lineTo(3.425f, 16.0f);
    p.lineTo(13.2f, 6.225f);
    p.lineTo(11.775f, 4.8f);
    p.lineTo(2.0f, 14.575f);
    p.lineTo(2.0f, 16.0f);
    p.closeSubPath();

    p.startNewSubPath(0.0f, 18.0f);
    p.lineTo(0.0f, 13.75f);
    p.lineTo(13.2f, 0.575f);
    p.cubicTo(13.4f, 0.391667f, 13.621f, 0.25f, 13.863f, 0.15f);
    p.cubicTo(14.105f, 0.05f, 14.359f, 0.0f, 14.625f, 0.0f);
    p.cubicTo(14.891f, 0.0f, 15.1493f, 0.05f, 15.4f, 0.15f);
    p.cubicTo(15.6507f, 0.25f, 15.8673f, 0.4f, 16.05f, 0.6f);
    p.lineTo(17.425f, 2.0f);
    p.cubicTo(17.625f, 2.18333f, 17.771f, 2.4f, 17.863f, 2.65f);
    p.cubicTo(17.955f, 2.9f, 18.0007f, 3.15f, 18.0f, 3.4f);
    p.cubicTo(18.0f, 3.66667f, 17.9543f, 3.921f, 17.863f, 4.163f);
    p.cubicTo(17.7717f, 4.405f, 17.6257f, 4.62567f, 17.425f, 4.825f);
    p.lineTo(4.25f, 18.0f);
    p.lineTo(0.0f, 18.0f);
    p.closeSubPath();

    p.startNewSubPath(12.475f, 5.525f);
    p.lineTo(11.775f, 4.8f);
    p.lineTo(13.2f, 6.225f);
    p.lineTo(12.475f, 5.525f);
    p.closeSubPath();

    p.applyTransform(juce::AffineTransform::scale(scale));
    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.fillPath(p);
}

void UIUtils::drawSineWave(juce::Graphics& g, juce::Rectangle<float> bounds, int n, juce::Colour c)
{
    const float x = bounds.getX();
    const float y = bounds.getY();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    juce::Path path;

    // Start at the left edge, vertically centered
    path.startNewSubPath(x, y + h * 0.5f);

    // Number of points to draw
    for (int i = 0; i <= w; ++i)
    {
        const float t = (float)i / (float)w;
        const float px = x + t * w;
        const float angle = t * n * juce::MathConstants<float>::twoPi;
        const float py = y + h * 0.5f - std::sin(angle) * (h * 0.5f);
        path.lineTo(px, py);
    }

    g.setColour(c);
    g.strokePath(path, juce::PathStrokeType(1.0f));
}

void UIUtils::drawPowerButton(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c, float scale)
{
    float r = 6;
    auto pi = juce::MathConstants<float>::pi;
    g.setColour(c);
    juce::Path p;
    p.addArc(bounds.getCentreX() - r, bounds.getCentreY() - r, r * 2, r * 2, 0.75f, 2.f * pi - 0.75f, true);
    p.startNewSubPath(bounds.getCentreX(), bounds.getCentreY() - r);
    p.lineTo(bounds.getCentreX(), bounds.getCentreY());
    p.applyTransform(juce::AffineTransform::scale(scale));
    g.strokePath(p, juce::PathStrokeType(2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void UIUtils::drawWheel(juce::Graphics& g, juce::Rectangle<float> bounds, float wheelnorm)
{
    g.setColour(COLOR_BEVEL());
    g.fillRoundedRectangle(bounds, 2.f);

    juce::ColourGradient grad(
        juce::Colour(0xff1f1f1f),
        bounds.getX(), bounds.getY(),
        juce::Colour(0xff1f1f1f),
        bounds.getX(), bounds.getBottom(),
        false
    );
    grad.addColour(0.5, juce::Colour(0xFF333333));
    bounds = bounds.reduced(2.f);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds, 2.f);

    auto h = bounds.getHeight() * 2;
    auto y = bounds.getY() - bounds.getHeight() / 2 + bounds.getHeight() / 2 * wheelnorm;
    g.saveState();
    g.reduceClipRegion(bounds.toNearestInt());
    int step = 10;
    for (int i = 0; i < step; ++i) {
        if (i == 5) {
            g.setColour(juce::Colour(0xffffffff).withAlpha(0.2f));
            g.drawHorizontalLine((int)(y + i * (h / step)), bounds.getX(), bounds.getRight());
            g.drawHorizontalLine((int)(y + i * (h / step) + 1), bounds.getX(), bounds.getRight());
        }
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.drawHorizontalLine((int)(y + i * (h / step)), bounds.getX(), bounds.getRight());
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawHorizontalLine((int)(y + i * (h / step) + 1), bounds.getX(), bounds.getRight());
    }
    g.restoreState();


    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.drawVerticalLine((int)bounds.getX(), bounds.getY(), bounds.getBottom());
    g.drawVerticalLine((int)bounds.getRight() - 1, bounds.getY(), bounds.getBottom());
}

void UIUtils::drawUndo(juce::Graphics& g, juce::Rectangle<float> area, bool invertx, juce::Colour color)
{
    auto bounds = area;
    auto thickness = 2.f;
    float left = bounds.getX();
    float right = bounds.getRight();
    float top = bounds.getCentreY() - 4;
    float bottom = bounds.getCentreY() + 4;
    float centerY = bounds.getCentreY();
    float shaftStart = right - 7;

    juce::Path arrowPath;
    // arrow head
    arrowPath.startNewSubPath(right, centerY);
    arrowPath.lineTo(shaftStart, top);
    arrowPath.startNewSubPath(right, centerY);
    arrowPath.lineTo(shaftStart, bottom);

    // shaft
    float radius = (bottom - centerY);
    arrowPath.startNewSubPath(right, centerY);
    arrowPath.lineTo(left + radius - 1, centerY);

    // semi circle
    arrowPath.startNewSubPath(left + radius, centerY);
    arrowPath.addArc(left, centerY, radius, radius, 2.0f * juce::MathConstants<float>::pi, juce::MathConstants<float>::pi);

    if (invertx) {
        juce::AffineTransform flipTransform = juce::AffineTransform::scale(-1.0f, 1.0f)
            .translated(bounds.getWidth(), 0);

        // First move the path to origin, apply transform, then move back
        arrowPath.applyTransform(juce::AffineTransform::translation(-bounds.getPosition()));
        arrowPath.applyTransform(flipTransform);
        arrowPath.applyTransform(juce::AffineTransform::translation(bounds.getPosition()));
    }

    g.setColour(color);
    g.strokePath(arrowPath, juce::PathStrokeType(thickness));
}

void UIUtils::drawClock(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color)
{
    g.setColour(color);
    g.drawEllipse(bounds, 1.f);
    auto cx = bounds.getCentreX();
    auto cy = bounds.getCentreY();
    g.drawLine(cx, cy, cx, cy - bounds.getHeight() * 0.25f);
    g.drawLine(cx, cy, cx + bounds.getWidth() * .25f, cy + bounds.getHeight() * .25f);
}

// mode = 0, straight, 1 = tripplet, 2 = dotted
void UIUtils::drawNote(juce::Graphics& g, juce::Rectangle<float> bounds, int mode, juce::Colour color)
{
    auto r = 3;
    g.setColour(color);
    g.fillEllipse(bounds.getCentreX() - r, bounds.getBottom() - r * 2, r * 2.f, r * 2.f);
    g.drawVerticalLine((int)bounds.getCentreX() + r - 1, bounds.getBottom() - 10 - r, bounds.getBottom() - r);

    g.setFont(juce::FontOptions(12.f));
    if (mode == 1)
        g.drawText("t", (int)bounds.getCentreX() + r + 2, (int)bounds.getBottom() - 12, 12, 12, juce::Justification::centredLeft);
    if (mode == 2) {
        g.setFont(juce::FontOptions(16.f));
        g.drawText(".", (int)bounds.getCentreX() + r + 2, (int)bounds.getBottom() - 16, 16, 16, juce::Justification::centredLeft);
    }
}

void UIUtils::drawChain(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color)
{
    juce::Path p;

    p.startNewSubPath(0.5f, 7.37234f);
    p.lineTo(0.5f, 4.80818f);
    p.cubicTo(0.517043f, 3.65918f, 0.945923f, 2.56357f, 1.69414f, 1.75762f);
    p.cubicTo(2.44236f, 0.95168f, 3.44995f, 0.5f, 4.4996f, 0.5f);
    p.cubicTo(5.54925f, 0.5f, 6.55685f, 0.951681f, 7.30506f, 1.75762f);
    p.cubicTo(8.05328f, 2.56357f, 8.48216f, 3.65918f, 8.4992f, 4.80817f);
    p.lineTo(8.4992f, 7.37234f);

    p.startNewSubPath(8.4992f, 12.6268f);
    p.lineTo(8.5f, 15.1918f);
    p.cubicTo(8.48296f, 16.3408f, 8.05408f, 17.4364f, 7.30586f, 18.2424f);
    p.cubicTo(6.55765f, 19.0483f, 5.55005f, 19.5f, 4.5004f, 19.5f);
    p.cubicTo(3.45075f, 19.5f, 2.44316f, 19.0483f, 1.69494f, 18.2424f);
    p.cubicTo(0.946723f, 17.4364f, 0.517842f, 16.3408f, 0.5008f, 15.1918f);
    p.lineTo(0.5f, 12.6268f);

    p.startNewSubPath(4.4996f, 6.4966f);
    p.lineTo(4.4996f, 13.5025f);


    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(color);
    g.strokePath(p, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
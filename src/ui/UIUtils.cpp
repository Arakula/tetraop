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

juce::String UIUtils::capitalize(juce::String str)
{
    if (str.isEmpty())
        return "";

    juce::StringArray parts;
    parts.addTokens(str, "_", "");

    for (auto& part : parts) {
        if (part.isNotEmpty()) {
            part = part.substring(0, 1).toUpperCase() + part.substring(1);
        }
    }

    return parts.joinIntoString(" ");
}

juce::String UIUtils::aliasModulator(const juce::String& input)
{
    static const std::map<juce::String, juce::String> aliasMap = {
        {"key", "Key Track"},
        {"vel", "Velocity"},
        {"mod", "Mod Wheel"},
        {"rand", "Random"},
        {"at", "After Touch"},
        {"bend", "Pitch Bend"},
        {"sus", "Sust. Pedal"},
        {"soft", "Soft Pedal"},
        {"exp", "Exp. Pedal"},
    };

    auto it = aliasMap.find(input);
    if (it != aliasMap.end())
        return it->second;

    return capitalize(input);
}

juce::String UIUtils::aliasParameter(const juce::String& pid)
{
    juce::String txt = pid;
    if (pid.contains("_sub_"))
        txt = pid.replace("_sub_", "_osc_");

    return capitalize(txt);
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

void UIUtils::drawPanel(Graphics& g, Rectangle<float> bounds, bool drawHeader, bool darker, Colour headerc)
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
        g.setColour(headerc == Colours::transparentBlack ? COLOR_PANEL_HEADER() : headerc);
        g.fillPath(p);

        //g.setColour(Colours::white.withAlpha(0.6f));
        //g.strokePath(p, PathStrokeType(1.f));

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

void UIUtils::drawLogo(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color)
{
    const juce::String svgText =
        R"svg(
            <svg width="35" height="35" viewBox="0 0 35 35" fill="none" xmlns="http://www.w3.org/2000/svg">
                <path d="M15.768 3C16.5378 1.66667 18.4623 1.66667 19.2321 3L22.2631 8.25C23.0329 9.58333 22.0707 11.25 20.5311 11.25H14.4689C12.9293 11.25 11.9671 9.58333 12.7369 8.25L15.768 3Z" fill="#D9D9D9"/>
                <path d="M15.768 32C16.5378 33.3333 18.4623 33.3333 19.2321 32L22.2631 26.75C23.0329 25.4167 22.0707 23.75 20.5311 23.75H14.4689C12.9293 23.75 11.9671 25.4167 12.7369 26.75L15.768 32Z" fill="#D9D9D9"/>
                <path d="M31.7326 18.7606C33.1343 18.0053 33.1343 15.9947 31.7326 15.2394L26.6987 12.5268C25.3663 11.8088 23.75 12.7738 23.75 14.2874L23.75 19.7126C23.75 21.2262 25.3663 22.1912 26.6987 21.4732L31.7326 18.7606Z" fill="#D9D9D9"/>
                <path d="M3.26735 18.7606C1.86568 18.0053 1.86568 15.9947 3.26736 15.2394L8.30126 12.5268C9.63374 11.8088 11.25 12.7738 11.25 14.2874L11.25 19.7126C11.25 21.2262 9.63373 22.1912 8.30126 21.4732L3.26735 18.7606Z" fill="#D9D9D9"/>
            </svg>
        )svg";

    Path path = gin::SVG::renderToPath(svgText, bounds);
    g.setColour(color);
    g.fillPath(path);
}

void UIUtils::drawSave(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color)
{
    const juce::String svgText =
        R"svg(
            <svg width="16" height="16" viewBox="0 0 16 16" fill="none" xmlns="http://www.w3.org/2000/svg">
<g clip-path="url(#clip0_226_15)">
<path d="M2 1C1.73478 1 1.48043 1.10536 1.29289 1.29289C1.10536 1.48043 1 1.73478 1 2V14C1 14.2652 1.10536 14.5196 1.29289 14.7071C1.48043 14.8946 1.73478 15 2 15H14C14.2652 15 14.5196 14.8946 14.7071 14.7071C14.8946 14.5196 15 14.2652 15 14V2C15 1.73478 14.8946 1.48043 14.7071 1.29289C14.5196 1.10536 14.2652 1 14 1H9.5C9.23478 1 8.98043 1.10536 8.79289 1.29289C8.60536 1.48043 8.5 1.73478 8.5 2V9.293L11.146 6.646C11.2399 6.55211 11.3672 6.49937 11.5 6.49937C11.6328 6.49937 11.7601 6.55211 11.854 6.646C11.9479 6.73989 12.0006 6.86722 12.0006 7C12.0006 7.13278 11.9479 7.26011 11.854 7.354L8.354 10.854C8.30755 10.9006 8.25238 10.9375 8.19163 10.9627C8.13089 10.9879 8.06577 11.0009 8 11.0009C7.93423 11.0009 7.86911 10.9879 7.80837 10.9627C7.74762 10.9375 7.69245 10.9006 7.646 10.854L4.146 7.354C4.09951 7.30751 4.06264 7.25232 4.03748 7.19158C4.01232 7.13084 3.99937 7.06574 3.99937 7C3.99937 6.93426 4.01232 6.86916 4.03748 6.80842C4.06264 6.74768 4.09951 6.69249 4.146 6.646C4.19249 6.59951 4.24768 6.56264 4.30842 6.53748C4.36916 6.51232 4.43426 6.49937 4.5 6.49937C4.56574 6.49937 4.63084 6.51232 4.69158 6.53748C4.75232 6.56264 4.80751 6.59951 4.854 6.646L7.5 9.293V2C7.5 1.46957 7.71071 0.960859 8.08579 0.585786C8.46086 0.210714 8.96957 0 9.5 0L14 0C14.5304 0 15.0391 0.210714 15.4142 0.585786C15.7893 0.960859 16 1.46957 16 2V14C16 14.5304 15.7893 15.0391 15.4142 15.4142C15.0391 15.7893 14.5304 16 14 16H2C1.46957 16 0.960859 15.7893 0.585786 15.4142C0.210714 15.0391 0 14.5304 0 14V2C0 1.46957 0.210714 0.960859 0.585786 0.585786C0.960859 0.210714 1.46957 0 2 0L4.5 0C4.63261 0 4.75979 0.0526784 4.85355 0.146447C4.94732 0.240215 5 0.367392 5 0.5C5 0.632608 4.94732 0.759785 4.85355 0.853553C4.75979 0.947322 4.63261 1 4.5 1H2Z" fill="white"/>
</g>
<defs>
<clipPath id="clip0_226_15">
<rect width="16" height="16" fill="white"/>
</clipPath>
</defs>
</svg>
        )svg";

    Path path = gin::SVG::renderToPath(svgText, bounds);
    g.setColour(color);
    g.fillPath(path);
}

void UIUtils::drawEllipsis(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color)
{
    const juce::String svgText =
        R"svg(
            <svg width="4" height="16" viewBox="0 0 4 16" fill="none" xmlns="http://www.w3.org/2000/svg">
            <circle cx="2" cy="2" r="2" fill="#D9D9D9"/>
            <circle cx="2" cy="8" r="2" fill="#D9D9D9"/>
            <circle cx="2" cy="14" r="2" fill="#D9D9D9"/>
            </svg>
        )svg";

    Path path = gin::SVG::renderToPath(svgText, bounds);
    g.setColour(color);
    g.fillPath(path);
}

void UIUtils::drawCPU(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color)
{
    const juce::String svgText =
        R"svg(
            <svg width="16" height="16" viewBox="0 0 16 16" fill="none" xmlns="http://www.w3.org/2000/svg">
            <path d="M3.2 12.8H12.8V3.2H3.2V12.8ZM9.6 14.4H6.4V16H4.8V14.4H2.4C2.18783 14.4 1.98434 14.3157 1.83431 14.1657C1.68429 14.0157 1.6 13.8122 1.6 13.6V11.2H0V9.6H1.6V6.4H0V4.8H1.6V2.4C1.6 2.18783 1.68429 1.98434 1.83431 1.83431C1.98434 1.68429 2.18783 1.6 2.4 1.6H4.8V0H6.4V1.6H9.6V0H11.2V1.6H13.6C13.8122 1.6 14.0157 1.68429 14.1657 1.83431C14.3157 1.98434 14.4 2.18783 14.4 2.4V4.8H16V6.4H14.4V9.6H16V11.2H14.4V13.6C14.4 13.8122 14.3157 14.0157 14.1657 14.1657C14.0157 14.3157 13.8122 14.4 13.6 14.4H11.2V16H9.6V14.4ZM4.8 4.8H11.2V11.2H4.8V4.8Z" fill="#EBEBEB"/>
            </svg>
        )svg";

    Path path = gin::SVG::renderToPath(svgText, bounds);
    g.setColour(color);
    g.fillPath(path);
}

void UIUtils::drawVoices(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color)
{
    const juce::String svgText =
        R"svg(
            <svg width="14" height="14" viewBox="0 0 14 14" fill="none" xmlns="http://www.w3.org/2000/svg">
            <path d="M6.91842 0C7.1067 2.48376e-05 7.28843 0.0691505 7.42913 0.194266C7.56983 0.319381 7.65972 0.491783 7.68175 0.678774L7.68713 0.768713V13.0681C7.68692 13.2641 7.61189 13.4525 7.47739 13.595C7.34289 13.7374 7.15906 13.8232 6.96347 13.8347C6.76788 13.8461 6.57528 13.7825 6.42504 13.6568C6.27479 13.531 6.17823 13.3526 6.15509 13.1581L6.14971 13.0681V0.768713C6.14971 0.564838 6.2307 0.369313 6.37486 0.225151C6.51902 0.0809892 6.71455 0 6.91842 0ZM3.84357 2.30614C4.04744 2.30614 4.24297 2.38713 4.38713 2.53129C4.53129 2.67545 4.61228 2.87098 4.61228 3.07485V10.762C4.61228 10.9659 4.53129 11.1614 4.38713 11.3055C4.24297 11.4497 4.04744 11.5307 3.84357 11.5307C3.63969 11.5307 3.44417 11.4497 3.3 11.3055C3.15584 11.1614 3.07485 10.9659 3.07485 10.762V3.07485C3.07485 2.87098 3.15584 2.67545 3.3 2.53129C3.44417 2.38713 3.63969 2.30614 3.84357 2.30614ZM9.99327 2.30614C10.1972 2.30614 10.3927 2.38713 10.5368 2.53129C10.681 2.67545 10.762 2.87098 10.762 3.07485V10.762C10.762 10.9659 10.681 11.1614 10.5368 11.3055C10.3927 11.4497 10.1972 11.5307 9.99327 11.5307C9.7894 11.5307 9.59387 11.4497 9.44971 11.3055C9.30555 11.1614 9.22456 10.9659 9.22456 10.762V3.07485C9.22456 2.87098 9.30555 2.67545 9.44971 2.53129C9.59387 2.38713 9.7894 2.30614 9.99327 2.30614ZM0.768713 4.61228C0.972589 4.61228 1.16811 4.69327 1.31228 4.83743C1.45644 4.98159 1.53743 5.17712 1.53743 5.38099V8.45585C1.53743 8.65972 1.45644 8.85525 1.31228 8.99941C1.16811 9.14357 0.972589 9.22456 0.768713 9.22456C0.564838 9.22456 0.369313 9.14357 0.225151 8.99941C0.0809892 8.85525 0 8.65972 0 8.45585V5.38099C0 5.17712 0.0809892 4.98159 0.225151 4.83743C0.369313 4.69327 0.564838 4.61228 0.768713 4.61228ZM13.0681 4.61228C13.2564 4.61231 13.4381 4.68143 13.5788 4.80655C13.7195 4.93166 13.8094 5.10406 13.8315 5.29105L13.8368 5.38099V8.45585C13.8366 8.65178 13.7616 8.84023 13.6271 8.9827C13.4926 9.12517 13.3088 9.2109 13.1132 9.22239C12.9176 9.23387 12.725 9.17023 12.5747 9.04448C12.4245 8.91872 12.3279 8.74034 12.3048 8.54579L12.2994 8.45585V5.38099C12.2994 5.17712 12.3804 4.98159 12.5246 4.83743C12.6687 4.69327 12.8643 4.61228 13.0681 4.61228Z" fill="#EBEBEB"/>
            </svg>
        )svg";

    Path path = gin::SVG::renderToPath(svgText, bounds);
    g.setColour(color);
    g.fillPath(path);
}

void UIUtils::drawPeak(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;
    p.startNewSubPath(0.0f, 11.5f);
    p.cubicTo(9.0f, 11.5f, 6.5f, 0.5f, 9.0f, 0.5f);
    p.cubicTo(11.5f, 0.5f, 9.0f, 11.5f, 18.0f, 11.5f);
    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved));
}

void UIUtils::drawLowShelf(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;
    p.startNewSubPath(0.0f, 13.0f);
    p.cubicTo(1.0f, 13.0f, 5.0f, 13.0f, 5.0f, 13.0f);
    p.cubicTo(9.0f, 13.0f, 9.0f, 0.f, 13.0f, 0.f);
    p.cubicTo(17.0f, 0.f, 18.0f, 0.f, 18.0f, 0.f);
    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved));
}

void UIUtils::drawHighShelf(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;
    p.startNewSubPath(18.0f, 13.0f);
    p.cubicTo(17.0f, 13.0f, 13.0f, 13.0f, 13.0f, 13.0f);
    p.cubicTo(9.0f, 13.0f, 9.0f, 0.f, 5.0f, 0.f);
    p.cubicTo(1.0f, 0.f, 0.0f, 0.f, 0.0f, 0.f);
    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved));
}

void UIUtils::drawHighpass(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;
    p.startNewSubPath(18.f, 0.f);
    p.cubicTo(10.f, 0.f, 8.f, 0.5f, 8.f, 0.f);
    p.cubicTo(5.f, 0.f, 5.f, 1.f, 4.f, 4.0f);
    p.cubicTo(3.f, 6.f, 0.f, 11.f, 0.f, 11.f);
    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved));
}

void UIUtils::drawLowpass(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour c)
{
    juce::Path p;
    p.startNewSubPath(0.0f, 0.f);
    p.cubicTo(8.0f, 0.f, 10.f, 0.f, 10.f, 0.f);
    p.cubicTo(13.0f, 0.f, 13.f, 1.f, 14.f, 4.0f);
    p.cubicTo(15.f, 6.f, 18.0f, 11.f, 18.0f, 11.f);
    p.applyTransform(juce::AffineTransform::translation(bounds.getX(), bounds.getY()));
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved));
}
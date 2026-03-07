#include "UIUtils.h"

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
    auto b = Rectangle<float>(bounds.getCentreX() - 5, bounds.getCentreY() - 5, 10.f, 10.f);
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
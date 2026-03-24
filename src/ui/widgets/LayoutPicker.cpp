#include "LayoutPicker.h"
#include "../../PluginEditor.h"

LayoutPicker::LayoutPicker(TetraOPAudioProcessorEditor& p)
	: editor(p)
{
    editor.audioProcessor.params.addParameterListener("layout", this);
}

LayoutPicker::~LayoutPicker()
{
    editor.audioProcessor.params.removeParameterListener("layout", this);
}

void LayoutPicker::parameterChanged(const juce::String&, float)
{
	MessageManager::callAsync([this]{ repaint(); });
}

void LayoutPicker::mouseDown(const MouseEvent& e)
{
    (void)e;
}

void LayoutPicker::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    (void)event;
    (void)wheel;
}

static void drawA_B_C_D(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx - r * 3, cy };
    Point<float> b = { cx - r, cy };
    Point<float> c = { cx + r, cy };
    Point<float> d = { cx + r * 3, cy };

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDCBA(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx, cy + r * 3};
    Point<float> b = { cx, cy + r };
    Point<float> c = { cx, cy - r};
    Point<float> d = { cx, cy - r * 3 };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, c));
    g.setColour(COLOR_C());
    g.drawLine(Line<float>(c, b));
    g.setColour(COLOR_B());
    g.drawLine(Line<float>(b, a));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDC_BA(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx - r, cy + r };
    Point<float> b = { cx - r, cy - r };
    Point<float> c = { cx + r, cy + r };
    Point<float> d = { cx + r, cy - r };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, c));
    g.setColour(COLOR_B());
    g.drawLine(Line<float>(b, a));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDC_B_A(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx - r * 2, cy + r };
    Point<float> b = { cx, cy + r };
    Point<float> c = { cx + r * 2, cy + r };
    Point<float> d = { cx + r * 2, cy - r };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, c));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDA_DB_DC(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx - r * 2, cy + r };
    Point<float> b = { cx, cy + r };
    Point<float> c = { cx + r * 2, cy + r };
    Point<float> d = { cx, cy - r };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, b));
    g.drawLine(a.getX(), a.getY(), a.getX(), a.getY() - r);
    g.drawLine(c.getX(), c.getY(), c.getX(), c.getY() - r);
    g.drawLine(a.getX(), a.getY() - r, c.getX(), c.getY() - r);

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawBA_CA_DA(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx, cy + r };
    Point<float> b = { cx - r*2, cy - r };
    Point<float> c = { cx, cy - r };
    Point<float> d = { cx + r * 2, cy - r };

    g.setColour(COLOR_C());
    g.drawLine(Line<float>(c, a));
    g.setColour(COLOR_B());
    g.drawLine(b.getX(), b.getY(), b.getX(), b.getY() + r);
    g.drawLine(b.getX() + r * 2, b.getY() + r, b.getX(), b.getY() + r);
    g.setColour(COLOR_D());
    g.drawLine(d.getX(), d.getY(), d.getX(), d.getY() + r);
    g.drawLine(d.getX() - r * 2, d.getY() + r, d.getX(), d.getY() + r);

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawA_CB_DC(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx - r, cy + r * 2 };
    Point<float> b = { cx + r, cy + r * 2 };
    Point<float> c = { cx + r, cy };
    Point<float> d = { cx + r, cy - r * 2 };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, c));
    g.setColour(COLOR_C());
    g.drawLine(Line<float>(c, b));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDC_CA_CB(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx - r, cy + r * 2 };
    Point<float> b = { cx + r, cy + r * 2 };
    Point<float> c = { cx, cy };
    Point<float> d = { cx, cy - r * 2 };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, c));
    g.setColour(COLOR_C());
    g.drawLine(Line<float>(c, a));
    g.drawLine(Line<float>(c, b));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDC_DB_BA_CA(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx, cy + r * 2 };
    Point<float> b = { cx - r, cy };
    Point<float> c = { cx + r, cy };
    Point<float> d = { cx, cy - r * 2 };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, c));
    g.drawLine(Line<float>(d, b));
    g.setColour(COLOR_C());
    g.drawLine(Line<float>(c, a));
    g.setColour(COLOR_B());
    g.drawLine(Line<float>(b, a));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

static void drawDB_CB_BA(Graphics& g, Rectangle<float> bounds, float scale)
{
    float r = 10 * scale;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    Point<float> a = { cx, cy + r * 2 };
    Point<float> b = { cx, cy };
    Point<float> c = { cx + r, cy - r * 2 };
    Point<float> d = { cx - r, cy - r * 2 };

    g.setColour(COLOR_D());
    g.drawLine(Line<float>(d, b));
    g.setColour(COLOR_C());
    g.drawLine(Line<float>(c, b));
    g.setColour(COLOR_B());
    g.drawLine(Line<float>(b, a));

    g.setColour(COLOR_A());
    g.fillRect(a.getX() - r / 2, a.getY() - r / 2, r, r);
    g.setColour(COLOR_B());
    g.fillRect(b.getX() - r / 2, b.getY() - r / 2, r, r);
    g.setColour(COLOR_C());
    g.fillRect(c.getX() - r / 2, c.getY() - r / 2, r, r);
    g.setColour(COLOR_D());
    g.fillRect(d.getX() - r / 2, d.getY() - r / 2, r, r);
}

void LayoutPicker::paint(Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(COLOR_BACKGROUND().darker(0.5f));
    g.fillRoundedRectangle(b.reduced(0.5f), 5.f);

    auto layout = (FmMatrix::Layout)editor.audioProcessor.params.getRawParameterValue("layout")->load();

    switch (layout)
    {
        case FmMatrix::A_B_C_D: 
            drawA_B_C_D(g, b, 1.f);
            break;
        case FmMatrix::DCBA: 
            drawDCBA(g, b, 1.f);
            break; 
        case FmMatrix::DC_BA: 
            drawDC_BA(g, b, 1.f);
            break; 
        case FmMatrix::DC_B_A: 
            drawDC_B_A(g, b, 1.f);
            break;
        case FmMatrix::DA_DB_DC: 
            drawDA_DB_DC(g, b, 1.f);
            break;
        case FmMatrix::BA_CA_DA: 
            drawBA_CA_DA(g, b, 1.f);
            break;
        case FmMatrix::A_CB_DC: 
            drawA_CB_DC(g, b, 1.f);
            break;
        case FmMatrix::DC_CA_CB: 
            drawDC_CA_CB(g, b, 1.f);
            break;
        case FmMatrix::DC_DB_BA_CA: 
            drawDC_DB_BA_CA(g, b, 1.f);
            break;
        case FmMatrix::DB_CB_BA: 
            drawDB_CB_BA(g, b, 1.f);
            break;
    }
}
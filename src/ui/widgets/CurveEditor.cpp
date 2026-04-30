#include "../../PluginEditor.h"
#include "CurveEditor.h"

CurveEditor::CurveEditor(TetraOPAudioProcessorEditor& e, Pattern* p, float pad, juce::Colour c, juce::Colour selc, bool islfo, bool use_multiselect)
    : usemultisel(use_multiselect)
    , islfo(islfo)
    , color(c)
    , pad(pad)
    , multisel(e.audioProcessor)
    , editor(e)
    , pattern(p)
{
    mpoint_radius = LFO_MPOINT_RADIUS;
    point_radius = LFO_POINT_RADIUS;
    point_hover = LFO_POINT_HOVER;
    multisel.color = selc;
    if (usemultisel) {
        multisel.setPattern(p);
    }
    setWantsKeyboardFocus(true);
}

CurveEditor::~CurveEditor()
{
}

void CurveEditor::setPattern(Pattern* p)
{
    if (usemultisel) {
        multisel.setPattern(p);
    }
    pattern = p;
    hoverPoint = 0;
    hoverMidpoint = 0;
    selectedPoint = 0;
    selectedMidpoint = 0;
}

void CurveEditor::mouseMove(const juce::MouseEvent& e)
{
    hoverPoint = 0;
    hoverMidpoint = 0;
    multisel.mouseHover = -1;
    auto pos = e.getPosition();

    // if currently dragging a point ignore mouse over events
    if (selectedPoint > 0 || selectedMidpoint > 0) {
        return;
    }

    // multi selection mouse over
    if (usemultisel) {
        multisel.mouseMove(e);
        if (multisel.mouseHover > -1) {
            repaint();
            return;
        }
    }

    int x = pos.x; int y = pos.y;
    hoverPoint = getHoveredPoint(x, y);
    if (hoverPoint == 0)
        hoverMidpoint = getHoveredMidpoint(x, y);

    repaint();
}

void CurveEditor::mouseExit(const juce::MouseEvent& e)
{
    (void)e;
    hoverPoint = 0;
    hoverMidpoint = 0;
}

void CurveEditor::mouseDown(const juce::MouseEvent& e)
{
    juce::Point pos = e.getPosition();
    int x = pos.x;
    int y = pos.y;

    if (e.mods.isLeftButtonDown()) {
        if (usemultisel && multisel.mouseHover > -1) {
            UIUtils::startUnboundedMouse(*this, e);
            multisel.mouseDown(e);
            repaint();
            return;
        }

        selectedPoint = getHoveredPoint(x, y);
        if (selectedPoint == 0)
            selectedMidpoint = getHoveredMidpoint(x, y);

        if (usemultisel && selectedPoint == 0 && selectedMidpoint == 0) {
            preSelectionStart = e.getPosition();
            preSelectionEnd = e.getPosition();
        }

        if (selectedPoint > 0 || selectedMidpoint > 0) {
            //editor.audioProcessor.undomgr->createUndo();

            if (selectedPoint > 0) {
                UIUtils::startUnboundedMouse(*this, e);
            }
            if (selectedMidpoint > 0) {
                origTension = getPoint(selectedMidpoint).tension;
                dragStartY = y;
                UIUtils::startUnboundedMouse(*this, e);
            }
        }

        repaint();
    }
}

void CurveEditor::mouseUp(const juce::MouseEvent& e)
{
    auto unbounded = UIUtils::stopUnboundedMouse(*this, e);

    if (selectedPoint > 0) { // finished dragging point
        // ----
    }
    else if (selectedMidpoint > 0) { // finished dragging midpoint, place cursor at midpoint
        if (unbounded && e.getDistanceFromDragStart() > 5)
        {
            int x = e.getMouseDownX();
            int y = (int)((1 - pattern->get_y_at((x - winx) / winw)) * winh + winy);

            juce::Desktop::getInstance().setMousePosition(localPointToGlobal(juce::Point<int>(x,y)));
        }
    }
    else if (usemultisel && preSelectionStart.x > -1 && (std::abs(preSelectionStart.x - preSelectionEnd.x) > 4 ||
        std::abs(preSelectionStart.y - preSelectionEnd.y) > 4))
    {
        multisel.makeSelection(e, preSelectionStart, preSelectionEnd);
    }
    else if (usemultisel && multisel.mouseHover > -1) {
        multisel.mouseUp(e);
    }
    else if (usemultisel && !multisel.selectionPoints.empty()) { // finished dragging selection
        multisel.clearSelection();
    }

    preSelectionStart = juce::Point<int>(-1, -1);
    selectedMidpoint = 0;
    selectedPoint = 0;

    if (onChange) onChange();
    repaint();
}

void CurveEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (usemultisel && multisel.mouseHover > -1 && e.mods.isRightButtonDown()) {
        return;
    }

    if (usemultisel && multisel.mouseHover > -1 && e.mods.isLeftButtonDown()) {
        multisel.mouseDrag(e, gridX, gridY);
        repaint();
        return;
    }

    if (e.mods.isRightButtonDown()) {
        return;
    }

    juce::Point pos = e.getPosition();
    int x = pos.x;
    int y = pos.y;

    if (selectedPoint) {
        float gridx = float(winw) / gridX;
        float gridy = float(winh) / gridY;
        float xx = (float)x;
        float yy = (float)y;
        if (isSnapping(e)) {
            xx = std::round((xx - winx) / gridx) * gridx + winx;
            yy = std::round((yy - winy) / gridy) * gridy + winy;
        }
        xx = (xx - winx) / winw;
        yy = (yy - winy) / winh;
        if (yy > 1) yy = 1.0f;
        if (yy < 0) yy = 0.0f;

        auto& point = getPoint(selectedPoint);
        point.y = 1 - yy;
        point.x = xx;
        if (point.x > 1) point.x = 1;
        if (point.x < 0) point.x = 0;
        auto idx = getPointIndex(point.id);
        if (idx < int (pattern->points.size() - 1)) {
            int idxnext = idx + 1;
            auto& next = pattern->points[idxnext];
            if (point.x >= next.x && point.x - next.x < 15 / winw) // keep the point snapped to the next point
                point.x = next.x - 1e-6f;
        }
        if (idx > 0) {
            int idxprev = idx - 1;
            auto& prev = pattern->points[idxprev];
            if (point.x <= prev.x && prev.x - point.x < 15 / winw)
                point.x = prev.x + 1e-6f;
        }

        pattern->sortPoints();
        pattern->buildSegments();
    }

    else if (selectedMidpoint) {
        int distance = y - dragStartY;
        auto& mpoint = getPoint(selectedMidpoint);
        float tension = (float)origTension + float(distance) / 500.f * -1;
        if (tension > 1) tension = 1;
        if (tension < -1) tension = -1;
        mpoint.tension = tension;
        pattern->buildSegments();
    }

    else if (usemultisel && preSelectionStart.x > -1) {
        preSelectionEnd = e.getPosition();
    }

    pattern->incrementVersion();
    repaint();
}

void CurveEditor::mouseDoubleClick(const juce::MouseEvent& e) {
    if (e.mods.isRightButtonDown()) {
        return;
    }

    //editor.audioProcessor.undomgr->createUndo();

    if (usemultisel && multisel.mouseHover > -1) {
        multisel.clearSelection();
        repaint();
        return;
    }

    int x = e.getPosition().x;
    int y = e.getPosition().y;
    uint64_t pt = getHoveredPoint((int)x, (int)y);
    uint64_t mid = getHoveredMidpoint((int)x, (int)y);

    if (pt) {
        pattern->removePoint(getPointIndex(pt));
        hoverPoint = 0;
        hoverMidpoint = 0;
    }
    else if (pt == 0 && mid > 0) {
        getPoint(mid).tension = 0;
    }
    else if (pt == 0 && mid == 0) {
        insertNewPoint(e);
    }

    pattern->buildSegments();
    pattern->incrementVersion();
    if (onChange) onChange();
    //editor.audioProcessor.undomgr->createUndo();
    repaint();
}

bool CurveEditor::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey && usemultisel && !multisel.selectionPoints.empty()) {
        //editor.audioProcessor.undomgr->createUndo();
        multisel.deleteSelectedPoints();
        if (onChange) onChange();
        return true;
    }

    // Let the parent class handle other keys (optional)
    return juce::Component::keyPressed(key);
}

void CurveEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    //editor.audioProcessor.undomgr->createUndo();
    if (onWheel) onWheel(e, wheel);
    if (!islfo) return;
    auto step = UIUtils::wheelStep(wheel, wheelAccum);
    if (step == 0) return;
    auto param = editor.audioProcessor.params.getParameter("lfo_grid");
    auto gridsz = (int)param->convertFrom0to1(param->getValue()) + step;
    param->setValueNotifyingHost(param->convertTo0to1((float)gridsz));
}

void CurveEditor::paint(juce::Graphics& g)
{
    auto bounds = viewBounds;
    if (useGrid) {
        drawGrid(g);
    }

    // draw pattern
    juce::Path path;
    auto pixels = bounds.getWidth();
    auto& pts = pattern->points;
    auto npts = (int)pts.size();
    bool pathStarted = false;

    // get_y_at samples one y per pixel column, so a vertical jump (two points
    // sharing the same x) at exactly t=0 or t=1 cannot be represented because
    // only one pixel column ever maps to that t. Draw those edge jumps explicitly.
    if (npts >= 2 && pts[0].x == pts[1].x && pts[0].x <= 0.0f) {
        auto px = bounds.getX();
        auto py0 = bounds.getY() + bounds.getHeight() * (1 - pts[0].y);
        auto py1 = bounds.getY() + bounds.getHeight() * (1 - pts[1].y);
        path.startNewSubPath(px, py0);
        path.lineTo(px, py1);
        pathStarted = true;
    }

    for (int x = 0; x < pixels; ++x) {
        float t = (float)x / (pixels - 1);
        auto px = bounds.getX() + x;
        auto py = bounds.getY() + bounds.getHeight() * (1 - pattern->get_y_at(t));
        if (!pathStarted) {
            path.startNewSubPath(px, py);
            pathStarted = true;
        }
        else {
            path.lineTo(px, py);
        }
    }

    if (npts >= 2 && pts[npts - 1].x == pts[npts - 2].x && pts[npts - 1].x >= 1.0f) {
        auto px = bounds.getX() + pixels - 1;
        auto py = bounds.getY() + bounds.getHeight() * (1 - pts[npts - 1].y);
        path.lineTo(px, py);
    }

    g.setColour(color);
    g.strokePath(path, juce::PathStrokeType(1.f));
    path.lineTo(bounds.getRight(), bounds.getBottom());
    path.lineTo(bounds.getX(), bounds.getBottom());
    path.closeSubPath();
    if (drawShade) {
        g.setColour(color.withAlpha(0.1f));
        g.fillPath(path);
    }

    if (usemultisel) {
        multisel.drawBackground(g);
    }

    // draw midpoints
    auto segs = pattern->getSegments();
    g.setColour(color);
    for (auto seg = segs.begin(); seg != segs.end(); ++seg) {
        if (!isCollinear(*seg) && seg->type != 0) {
            auto xy = getMidpointXY(*seg);
            g.fillEllipse((float)(xy[0] - mpoint_radius), (float)(xy[1] - mpoint_radius), mpoint_radius * 2.f, mpoint_radius * 2.f);
        }
    }

    // draw lfo points
    for (auto& point : pattern->points) {
        auto px = bounds.getX() + bounds.getWidth() * point.x;
        auto py = bounds.getY() + bounds.getHeight() * (1 - point.y);
        g.setColour(COLOR_BEVEL());
        g.fillEllipse((px - point_radius), (py - point_radius), (point_radius * 2), (point_radius * 2));
        g.setColour(juce::Colour(color));
        g.drawEllipse((px - point_radius), (py - point_radius), (point_radius * 2), (point_radius * 2), 2.f);
    }

    // draw hover point
    g.setColour(juce::Colour(color).withAlpha(0.5f));
    if (selectedPoint == 0 && selectedMidpoint == 0 && hoverPoint > 0) {
        auto& point = getPoint(hoverPoint);
        auto xx = point.x * winw + winx;
        auto yy = (1 - point.y) * winh + winy;
        g.fillEllipse((xx - point_hover), (yy - point_hover), point_hover * 2.f, point_hover * 2.f);
    }

    // draw selected point
    g.setColour(COLOR_ACTIVE());
    if (selectedPoint != 0) {
        auto& point = getPoint(selectedPoint);
        auto xx = point.x * winw + winx;
        auto yy = (1 - point.y) * winh + winy;
        g.fillEllipse((xx - point_radius), (yy - point_radius), point_radius * 2.f, point_radius * 2.f);
    }

    // draw hovered midpoint
    g.setColour(juce::Colour(color).withAlpha(0.5f));
    if (selectedPoint == 0 && selectedMidpoint == 0 && hoverMidpoint != 0) {
        auto index = getPointIndex(hoverMidpoint);
        auto next = index + 1;
        auto& seg = pattern->segments[next];
        auto xy = getMidpointXY(seg);
        g.fillEllipse((float)xy[0] - point_hover, (float)xy[1] - point_hover, point_hover * 2.f, point_hover * 2.f);
    }

    // draw selected midpoint
    g.setColour(COLOR_ACTIVE());
    if (selectedMidpoint != 0) {
        auto index = getPointIndex(selectedMidpoint);
        auto next = index + 1;
        auto& seg = pattern->segments[next];
        auto xy = getMidpointXY(seg);
        g.fillEllipse((float)xy[0] - mpoint_radius, (float)xy[1] - mpoint_radius, mpoint_radius * 2.f, mpoint_radius * 2.f);
    }

    if (usemultisel) {
        drawPreSelection(g);
        multisel.draw(g);
    }

    if (drawBasicSeek) {
        auto x = viewBounds.getX() + viewBounds.getWidth() * seekPos;
        auto y = viewBounds.getY() + (1 - pattern->get_y_at(seekPos)) * viewBounds.getHeight();
        g.setColour(multisel.color);
        g.drawEllipse(x - point_radius, y - mpoint_radius, point_radius * 2, point_radius * 2, 1.f);
        g.drawLine(x, y, x, viewBounds.getBottom());
    }

    if (drawSeek) {
        juce::Path seek;
        auto seekwidth = 30;
        auto start = seekPos * bounds.getWidth();
        int xStart = std::max<int>(0, (int)(start - seekwidth));
        int xEnd = std::min<int>((int)start, (int)pixels - 1);

        auto points = std::vector<juce::Point<float>>{};
        for (int x = xStart; x < xEnd; ++x) {
            float norm = x / (bounds.getWidth() - 1); // [0,1] across bounds
            float px = bounds.getX() + x;
            float py = bounds.getY() + bounds.getHeight() * (1 - pattern->get_y_at(norm));
            points.push_back({ px, py });
        }

        if (points.size()) {
            juce::Colour c = juce::Colour(color).brighter(1.0f);
            for (int i = 0; i < int (points.size()) - 1; ++i) {
                g.setColour(c.withAlpha((i + 1) / (float)points.size()));
                auto& p1 = points[i];
                auto& p2 = points[i + 1];
                g.drawLine(p1.x, p1.y, p2.x, p2.y, 2.f);
            }
        }
    }
}

void CurveEditor::clearSelection()
{
    multisel.clearSelection();
    hoverPoint = 0;
    selectedPoint = 0;
    selectedMidpoint = 0;
    hoverMidpoint = 0;
}

void CurveEditor::drawPreSelection(juce::Graphics& g)
{
    if (preSelectionStart.x == -1 || (preSelectionStart.x == preSelectionEnd.x && preSelectionStart.y == preSelectionEnd.y))
        return;

    int x1 = std::clamp(preSelectionStart.x, 0, getWidth());
    int y1 = std::clamp(preSelectionStart.y, 0, getHeight());
    int x2 = std::clamp(preSelectionEnd.x, 0, getWidth());
    int y2 = std::clamp(preSelectionEnd.y, 0, getHeight());

    int x = std::min(x1, x2);
    int y = std::min(y1, y2);
    int w = std::abs(x2 - x1);
    int h = std::abs(y2 - y1);

    auto bounds = juce::Rectangle<int>(x, y, w, h);

    g.setColour(multisel.color);
    g.drawRect(bounds);
    g.setColour(multisel.color.withAlpha(0.25f));
    g.fillRect(bounds);
}

void CurveEditor::drawGrid(juce::Graphics& g) const
{
    double gridx = double(winw) / gridX;
    double gridy = double(winh) / (gridY >= 16 ? 4 : gridY);

    g.setColour(juce::Colours::black.withAlpha(0.2f));
    if (gridX == 128) {
        for (int i = 0; i < gridX + 1; ++i) {
            int noteInOctave = i % 12;

            bool isBlack = noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                noteInOctave == 8 ||noteInOctave == 10;

            if (isBlack) {
                float x = (float)(winx + gridx * i);
                g.fillRect(x, winy, (float)gridx, winh);
            }
        }
    }

    for (int i = 0; i < gridX + 1; ++i) {
        if (gridX == 128)
            g.setColour(juce::Colours::white.withAlpha((i % 12 == 0 ? 0.1f : 0.025f)));
        else
            g.setColour(juce::Colours::white.withAlpha(0.05f));
        float x = (float)(winx + gridx * i);
        g.drawLine(x, (float)winy, x, (float)winy + winh);
    }
    for (int i = 0; i < gridY + 1; ++i) {
        float y = (float)(winy + gridy * i);
        g.drawLine((float)winx, y, (float)winx + winw, y);
    }
}

void CurveEditor::resized()
{
    viewBounds = getLocalBounds().toFloat().reduced(pad);
    winx = viewBounds.getX();
    winy = viewBounds.getY();
    winw = viewBounds.getWidth();
    winh = viewBounds.getHeight();
    multisel.setViewBounds((int)winx, (int)winy, (int)winw, (int)winh);
}

void CurveEditor::insertNewPoint(const juce::MouseEvent& e)
{
    float px = (float)e.x;
    float py = (float)e.y;
    if (isSnapping(e)) {
        float gridx = float(winw) / gridX;
        float gridy = float(winh) / gridY;
        px = std::round(float(px - winx) / gridx) * gridx + winx;
        py = std::round(float(py - winy) / gridy) * gridy + winy;
    }
    px = float(px - winx) / (float)winw;
    py = 1 - float(py - winy) / (float)winh;
    if (px >= 0 && px <= 1 && py >= 0 && py <= 1) { // point in env window
        pattern->insertPoint(px, py, 0.f, pattern->points.size() ? pattern->points[0].type : 1);
        pattern->sortPoints(); // keep things consistent, avoids reorders later
    }
    pattern->buildSegments();
}

uint64_t CurveEditor::getHoveredPoint(int x, int y)
{
    auto points = pattern->points;
    for (auto i = 0; i < int (points.size()); ++i) {
        auto xx = (int)(points[i].x * winw + winx);
        auto yy = (int)((1 - points[i].y) * winh + winy);
        if (pointInRect(x, y, xx - (int)point_hover, yy - (int)point_hover, (int)point_hover * 2, (int)point_hover * 2) &&
            !multisel.isSelected(points[i].id))
        {
            return points[i].id;
        }
    }
    return 0;
}

uint64_t CurveEditor::getHoveredMidpoint(int x, int y)
{
    auto segs = pattern->getSegments();
    for (auto i = 0; i < int (segs.size()); ++i) {
        auto& seg = segs[i];
        auto xy = getMidpointXY(seg);
        if (!isCollinear(seg) && seg.type != 0 && pointInRect(x, y,
            (int)(xy[0] - point_hover), (int)(xy[1] - point_hover), int(point_hover * 2), (int)(point_hover * 2)))
        {
            return getPointFromSegmentIndex(i).id;
        }
    }
    return 0;
}

Pattern::PPoint& CurveEditor::getPoint(uint64_t id)
{
    for (auto& point : pattern->points) {
        if (point.id == id) {
            return point;
        }
    }
    return dummyPoint;
}

int CurveEditor::getPointIndex(uint64_t id)
{
    for (int i = 0; i < int (pattern->points.size()); ++i) {
        if (pattern->points[i].id == id) {
            return i;
        }
    }
    return 0;
}

std::vector<double> CurveEditor::getMidpointXY(Pattern::Segment seg)
{
    float x = (std::max(seg.x1, 0.0f) + std::min(seg.x2, 1.0f)) * 0.5f;
    float y = (seg.type > 1 && seg.type != Pattern::SinCurve) && seg.x1 >= 0.0 && seg.x2 <= 1.0
        ? (seg.y1 + seg.y2) / 2
        : 1 - pattern->get_y_at(x);

    return {
        x * winw + winx,
        y * winh + winy
    };
}

// Midpoint index is derived from segment nu
// there is an extra segment before the first point
// so the matching pattern point to each midpoint is midpoint - 1
Pattern::PPoint& CurveEditor::getPointFromSegmentIndex(int segindex)
{
    auto size = (int)pattern->points.size();
    auto index = segindex == 0 ? size - 1 : segindex - 1;

    if (index >= size)
        index -= size;

    return pattern->points[index];
}

bool CurveEditor::isSnapping(const juce::MouseEvent& e) const
{
    return (snap && !e.mods.isShiftDown()) || (!snap && e.mods.isShiftDown());
}

bool CurveEditor::isCollinear(Pattern::Segment seg)
{
    return std::fabs(seg.x1 - seg.x2) < 0.01 || std::fabs(seg.y1 - seg.y2) < 0.01;
}

bool CurveEditor::pointInRect(int x, int y, int xx, int yy, int w, int h)
{
    return x >= xx && x <= xx + w && y >= yy && y <= yy + h;
}

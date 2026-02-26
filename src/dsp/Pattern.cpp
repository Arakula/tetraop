/*
  ==============================================================================

    Pattern.cpp
    Author:  tiagolr

  ==============================================================================
*/

#include "Pattern.h"
#include <cmath>
#include <algorithm>
#include "../PluginProcessor.h"

std::vector<Pattern::PPoint> Pattern::copy_pattern;

Pattern::Pattern()
{
    index = -1;
    incrementVersion();
}

Pattern::Pattern(int i)
{
    index = i;
    incrementVersion();
}

void Pattern::incrementVersion()
{
    versionID = versionIDCounter;
    versionIDCounter += 1;
}

void Pattern::sortPoints()
{
    std::sort(points.begin(), points.end(), [](const PPoint& a, const PPoint& b) {
        return a.x < b.x;
    });
}

void Pattern::setTension(float t)
{
    tensionMult.store(t);
}

int Pattern::insertPoint(float x, float y, float tension, int type, int role)
{
    auto id = pointsIDCounter;
    pointsIDCounter += 1;
    const PPoint p = { id, x, y, tension, type, role };

    // Find the insert position ordered by x
    auto insertPos = std::lower_bound(points.begin(), points.end(), p,
        [](const PPoint& a, const PPoint& b) {
            return a.x < b.x;
        });

    auto pidx = points.insert(insertPos, p);
    return (int)std::distance(points.begin(), pidx);
};

void Pattern::removePoint(float x, float y)
{
    for (size_t i = 0; i < points.size(); ++i) {
        if (points[i].x == x && points[i].y == y) {
            points.erase(points.begin() + i);
            return;
        }
    }
}

void Pattern::removePoint(int i) {
    points.erase(points.begin() + i);
}

void Pattern::removePointsInRange(float x1, float x2)
{
    for (auto i = points.begin(); i != points.end(); ++i) {
        if (i->x >= x1 && i->x <= x2) {
            i = points.erase(i);
            removePointsInRange(x1, x2);
            return;
        }
    }
}

void Pattern::invert()
{
    for (auto i = points.begin(); i != points.end(); ++i) {
        i->y = 1 - i->y;
    }
    incrementVersion();
};

void Pattern::reverse()
{
    std::reverse(points.begin(), points.end());

    float t0 = !points.empty() ? points[0].tension : 0.0f;
    int type0 = !points.empty() ? points[0].type : 1;
    for (size_t i = 0; i < points.size(); ++i) {
        auto& p = points[i];
        p.x = 1 - p.x;
        if (i < points.size() - 1) {
            p.tension = points[i + 1].tension * -1;
            p.type = points[i + 1].type;
        }
        else {
            p.tension = t0 * -1;
            p.type = type0;
        }
    }
    incrementVersion();
};

void Pattern::rotate(float x) {
    if (x > 1.0f) x = 1.0f;
    if (x < -1.0f) x = -1.0f;
    for (auto p = points.begin(); p != points.end(); ++p) {
        if (p->x == 0.0f) p->x += 1e-7f; // FIX - distinguish 1.0 and 0.0 points
        if (p->x == 1.0f) p->x -= 1e-7f; //
        p->x += x;
        if (p->x < 0.0f) p->x += 1.0f;
        if (p->x > 1.0f) p->x -= 1.0f;
    }
    sortPoints();
    incrementVersion();
}

void Pattern::floatPattern()
{
    auto pts = points;
    for (auto& p : pts) {
        insertPoint(p.x + 1.0f, p.y, p.tension, p.type, p.role);
    }

    for (auto& p : points) {
        p.x /= 2.0f;
    }

    incrementVersion();
}

void Pattern::clear()
{
    std::lock_guard<std::mutex> lock(pointsmtx);
    points.clear();
    incrementVersion();
}

void Pattern::buildSegments()
{
    std::vector<PPoint> pts;
    {
        std::lock_guard<std::mutex> lock(pointsmtx);
        pts = points;
    }
    // add ghost points outside the 0..1 boundary
    // allows the pattern to repeat itself and rotate seamlessly
    if (pts.size() == 0) {
        pts.push_back({0, -1.0f, 0.5f, 0.0f, 1});
        pts.push_back({0, 2.0f, 0.5f, 0.0f, 1});
    }
    else if (pts.size() == 1) {
        pts.insert(pts.begin(), {0, -1.0f, pts[0].y, 0.0f, 1});
        pts.push_back({0, 2.0f, pts[0].y, 0.0f, 1});
    }
    else {
        auto p1 = pts[0];
        auto p2 = pts[pts.size()-1];
        pts.insert(pts.begin(), {0, p2.x - 1.0f, p2.y, p2.tension, p2.type});
        pts.push_back({0, p1.x + 1.0f, p1.y, p1.tension, p1.type});
    }

    std::lock_guard<std::mutex> lock(mtx); // prevents crash while reading Y from another thread
    segments.clear();
    for (size_t i = 0; i < pts.size() - 1; ++i) {
        auto p1 = pts[i];
        auto p2 = pts[i + 1];
        segments.push_back({p1.x, p2.x, p1.y, p2.y, p1.tension, 0, p1.type});
    }
}

// thread safe get segments
// prevents getting segments during clear
std::vector<Pattern::Segment> Pattern::getSegments()
{
    std::lock_guard<std::mutex> lock(mtx);
    return segments;
}

void Pattern::loadSine() {
    clear();
    insertPoint(0, 0, 0, PointType::SinCurve);
    insertPoint(0.5f, 1.f, 0, PointType::SinCurve);
}

void Pattern::loadTriangle() {
    clear();
    insertPoint(0, 0, 0, 1);
    insertPoint(0.5f, 1.f, 0, 1);
};

void Pattern::copyFrom(Pattern& p)
{
    points = p.points;
    incrementVersion();
    buildSegments();
}

void Pattern::copy()
{
    copy_pattern = points;
}

void Pattern::paste()
{
    if (copy_pattern.size() > 0) {
        points = copy_pattern;
        incrementVersion();
    }
}

float Pattern::get_y_curve(Segment seg, float x)
{
    auto rise = seg.y1 > seg.y2;
    auto tmult = tensionMult.load();
    auto ten = seg.tension + (rise ? -tmult : tmult);
    if (!rise) ten *= -1;
    if (ten > 1) ten = 1;
    if (ten < -1) ten = -1;
    auto pwr = pow(1.1f, std::fabs(ten * POWER_CURVE_POWER));

    if (seg.x1 == seg.x2)
        return seg.y2;

    float t = (x - seg.x1) / (seg.x2 - seg.x1);
    t = ten >= 0
        ? std::pow(t, pwr)
        : t = 1.0f - std::pow(1.0f - t, pwr);

    return t * (seg.y2 - seg.y1) + seg.y1;
}

float Pattern::get_y_sincurve(Segment seg, float x)
{
    auto rise = seg.y1 > seg.y2;
    auto tmult = tensionMult.load();
    auto ten = seg.tension + (rise ? -tmult : tmult);
    if (!rise) ten *= -1;
    if (ten > 1) ten = 1;
    if (ten < -1) ten = -1;
    auto pwr = pow(1.1f, std::fabs(ten * POWER_CURVE_POWER));

    if (seg.x1 == seg.x2)
        return seg.y2;

    float t = (x - seg.x1) / (seg.x2 - seg.x1);
    t = 0.5f - 0.5f * std::cos(PI * t);

    t = ten >= 0
        ? std::pow(t, pwr)
        : t = 1.0f - std::pow(1.0f - t, pwr);

    return t * (seg.y2 - seg.y1) + seg.y1;
}

int Pattern::getWaveCount(Segment seg)
{
    if (seg.type == PointType::Pulse) return (int)(std::max(std::floor(std::pow(seg.tension,(float)2) * 100), 1.0f));
    if (seg.type == PointType::Wave) return (int)(std::floor(std::fabs(std::pow(seg.tension, (float)2) * 100) + 1) - 1);
    if (seg.type == PointType::Triangle) return (int)(std::floor(std::fabs(std::pow(seg.tension, (float)2) * 100) + 1) - 1.0f);
    if (seg.type == PointType::Stairs) return (int)(std::max(std::floor(std::pow(seg.tension, (float)2) * 150), 2.f));
    if (seg.type == PointType::SmoothSt) return (int)(std::max(std::floor(std::pow(seg.tension, (float)2) * 150), 1.0f));
    return 0;
}

float Pattern::get_y_scurve(Segment seg, float x)
{
  auto rise = seg.y1 > seg.y2;
  auto tmult = tensionMult.load();
  auto ten = seg.tension + (rise ? -tmult : tmult);
  if (ten > 1) ten = 1;
  if (ten < -1) ten = -1;
  auto pwr = pow(1.1f, std::fabs(ten * POWER_CURVE_POWER));

  float xx = (seg.x2 + seg.x1) / 2;
  float yy = (seg.y2 + seg.y1) / 2;

  if (seg.x1 == seg.x2)
    return seg.y2;

  if (x < xx && ten >=0)
    return std::pow((x - seg.x1) / (xx - seg.x1), pwr) * (yy - seg.y1) + seg.y1;

  if (x < xx && ten < 0)
    return -1 * (std::pow(1 - (x - seg.x1) / (xx - seg.x1), pwr) - 1) * (yy - seg.y1) + seg.y1;

  if (x >= xx && ten >= 0)
    return -1 * (std::pow(1 - (x - xx) / (seg.x2 - xx), pwr) - 1) * (seg.y2 - yy) + yy;

   return std::pow((x - xx) / (seg.x2 - xx), pwr) * (seg.y2 - yy) + yy;
}

float Pattern::get_y_pulse(Segment seg, float x)
{
  float t = std::max(std::floor(std::pow(seg.tension,(float)2) * 100), 1.0f); // num waves

  if (x == seg.x2)
    return seg.y2;

  float cycle_width = (seg.x2 - seg.x1) / t;
  float x_in_cycle = cycle_width == 0.0f ? 0.0f : std::fmod((x - seg.x1), cycle_width);
  return x_in_cycle < cycle_width / 2
    ? (seg.tension >= 0 ? seg.y1 : seg.y2)
    : (seg.tension >= 0 ? seg.y2 : seg.y1);
}

float Pattern::get_y_wave(Segment seg, float x)
{
  float t = 2 * std::floor(std::fabs(std::pow(seg.tension,(float)2) * 100) + 1) - 1; // wave num
  float amp = (seg.y2 - seg.y1) / 2;
  float vshift = seg.y1 + amp;
  float freq = t * 2 * PI / (2 * (seg.x2 - seg.x1));
  return -amp * cos(freq * (x - seg.x1)) + vshift;
}

float Pattern::get_y_triangle(Segment seg, float x)
{
  float tt = 2 * std::floor(std::fabs(std::pow(seg.tension, (float)2) * 100) + 1) - 1.0f;// wave num
  float amp = seg.y2 - seg.y1;
  float t = (seg.x2 - seg.x1) * 2 / tt;
  return amp * (2 * std::fabs((x - seg.x1) / t - std::floor(1.f/2.f + (x - seg.x1) / t))) + seg.y1;
}

float Pattern::get_y_stairs(Segment seg, float x)
{
  float t = std::max(std::floor(std::pow(seg.tension, (float)2) * 150), 2.f); // num waves
  float step_size = 0.f;
  float step_index = 0.f;
  float y_step_size = 0.f;

  if (seg.tension >= 0) {
    step_size = (seg.x2 - seg.x1) / t;
    step_index = std::floor((x - seg.x1) / step_size);
    y_step_size = (seg.y2 - seg.y1) / (t-1);
  }
  else {
    step_size = (seg.x2 - seg.x1) / (t-1);
    step_index = ceil((x - seg.x1) / step_size);
    y_step_size = (seg.y2 - seg.y1) / t;
  }

  if (x == seg.x2)
    return seg.y2;

  return seg.y1 + step_index * y_step_size;
}

float Pattern::get_y_smooth_stairs(Segment seg, float x)
{
  float pwr = 4;
  float t = std::max(std::floor(std::pow(seg.tension, (float)2) * (float)150), 1.0f); // num waves

  float gx = (seg.x2 - seg.x1) / t; // gridx
  float gy = (seg.y2 - seg.y1) / t; // gridy
  float step_index = std::floor((x - seg.x1) / gx);

  float xx1 = seg.x1 + gx * step_index;
  float xx2 = seg.x1 + gx * (step_index + 1);
  float xx = (xx1 + xx2) / 2;

  float yy1 = seg.y1 + gy * step_index;
  float yy2 = seg.y1 + gy * (step_index + 1);
  float yy = (yy1 + yy2) / 2;

  if (seg.x1 == seg.x2)
    return seg.y2;

  if (x < xx && seg.tension >= 0)
    return std::pow((x - xx1) / (xx - xx1), pwr) * (yy - yy1) + yy1;

  if (x < xx && seg.tension < 0)
    return -1 * (std::pow(1 - (x - xx1) / (xx - xx1), pwr) - 1) * (yy - yy1) + yy1;

  if (x >= xx && seg.tension >= 0)
    return -1 * (std::pow(1 - (x - xx) / (xx2 - xx), pwr) - 1) * (yy2 - yy) + yy;

  return std::pow((x - xx) / (xx2 - xx), pwr) * (yy2 - yy) + yy;
}


float Pattern::get_y_at(float x)
{
    std::lock_guard<std::mutex> lock(mtx); // prevents crash while building segments
    int low = 0;
    int high = static_cast<int>(segments.size()) - 1;

    // binary search the segment containing x
    while (low <= high) {
        int mid = (low + high) / 2;
        const auto& seg = segments[mid];

        if (x < seg.x1) {
            high = mid - 1;
        } else if (x > seg.x2) {
            low = mid + 1;
        } else {
            if (seg.type == PointType::Hold) return seg.y1; // hold
            if (seg.type == PointType::Curve) return get_y_curve(seg, x);
            if (seg.type == PointType::SinCurve) return get_y_sincurve(seg, x);
            if (seg.type == PointType::SCurve) return get_y_scurve(seg, x);
            if (seg.type == PointType::Pulse) return get_y_pulse(seg, x);
            if (seg.type == PointType::Wave) return get_y_wave(seg, x);
            if (seg.type == PointType::Triangle) return get_y_triangle(seg, x);
            if (seg.type == PointType::Stairs) return get_y_stairs(seg, x);
            if (seg.type == PointType::SmoothSt) return get_y_smooth_stairs(seg, x);
            return -1;
        }
    }

    return -1;
}

void Pattern::createUndo()
{
    if (undoStack.size() > 100) {
        undoStack.erase(undoStack.begin());
    }
    undoStack.push_back(points);
    redoStack.clear();
}
void Pattern::undo()
{
    if (undoStack.empty())
        return;

    redoStack.push_back(points);
    points = undoStack.back();
    undoStack.pop_back();

    incrementVersion();
    buildSegments();
}

void Pattern::redo()
{
    if (redoStack.empty())
        return;

    undoStack.push_back(points);
    points = redoStack.back();
    redoStack.pop_back();

    incrementVersion();
    buildSegments();
}

void Pattern::clearUndo()
{
    undoStack.clear();
    redoStack.clear();
}

bool Pattern::comparePoints(const std::vector<PPoint>& a, const std::vector<PPoint>& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].id != b[i].id ||
            a[i].x != b[i].x ||
            a[i].y != b[i].y ||
            a[i].tension != b[i].tension ||
            a[i].type != b[i].type) {
            return false;
        }
    }
    return true;
}

std::string Pattern::serialize()
{
    std::lock_guard<std::mutex> lock(pointsmtx);
    std::ostringstream oss;
    for (const auto& point : points) {
        oss << point.x << " " << point.y << " " << point.tension << " " << point.type << " " << point.role << " ";
    }
    return oss.str();
}

void Pattern::unserialize(std::string pts)
{
    {
        std::lock_guard<std::mutex> lock(pointsmtx);
        points.clear();
        float x, y, tension;
        int type;
        int role;
        std::istringstream iss(pts);
        while (iss >> x >> y >> tension >> type >> role) {
            insertPoint(x, y, tension, type, role);
        }
        sortPoints();
    }
    buildSegments();
    incrementVersion();
}
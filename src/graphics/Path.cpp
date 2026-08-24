#include "Path.h"

#include <algorithm>

namespace setu {
namespace graphics {

namespace {

// Control-point offset that turns a cubic Bezier into a quarter circle:
// 4/3 * (sqrt(2) - 1). Getting this constant wrong is the classic way to end up
// with corners that look slightly square or slightly bulged.
constexpr float KAPPA = 0.5522847498307933f;

} // namespace

void Path::reset() {
    mVerbs.clear();
    mPoints.clear();
    mFillType = FillType::WINDING;
}

void Path::addPoint(float x, float y) {
    mPoints.push_back(x);
    mPoints.push_back(y);
}

void Path::moveTo(float x, float y) {
    mVerbs.push_back(Verb::MOVE_TO);
    addPoint(x, y);
}

void Path::lineTo(float x, float y) {
    mVerbs.push_back(Verb::LINE_TO);
    addPoint(x, y);
}

void Path::cubicTo(float x1, float y1, float x2, float y2, float x3, float y3) {
    mVerbs.push_back(Verb::CUBIC_TO);
    addPoint(x1, y1);
    addPoint(x2, y2);
    addPoint(x3, y3);
}

void Path::close() {
    mVerbs.push_back(Verb::CLOSE);
}

void Path::addRect(const RectF& rect) {
    moveTo(rect.left, rect.top);
    lineTo(rect.right, rect.top);
    lineTo(rect.right, rect.bottom);
    lineTo(rect.left, rect.bottom);
    close();
}

void Path::addRoundRect(const RectF& rect, float rx, float ry) {
    const float radii[8] = { rx, ry, rx, ry, rx, ry, rx, ry };
    addRoundRect(rect, radii);
}

void Path::addRoundRect(const RectF& rect, const float radii[8]) {
    const float w = rect.width();
    const float h = rect.height();
    if (w <= 0.0f || h <= 0.0f) return;

    float r[8];
    for (int i = 0; i < 8; ++i) r[i] = (std::max)(0.0f, radii[i]);

    // Skia's scaleRadii: find the tightest edge and shrink every radius by the
    // same factor, so the shape stays a rounded rectangle instead of folding in
    // on itself when a layout asks for corners bigger than the view.
    float scale = 1.0f;
    const float edges[4][2] = {
        { r[0] + r[2], w }, // top:    tlX + trX
        { r[3] + r[5], h }, // right:  trY + brY
        { r[4] + r[6], w }, // bottom: brX + blX
        { r[7] + r[1], h }  // left:   blY + tlY
    };
    for (const auto& edge : edges) {
        if (edge[0] > edge[1] && edge[0] > 0.0f) {
            scale = (std::min)(scale, edge[1] / edge[0]);
        }
    }
    if (scale < 1.0f) {
        for (int i = 0; i < 8; ++i) r[i] *= scale;
    }

    const float l = rect.left, t = rect.top, rt = rect.right, b = rect.bottom;
    const float tlX = r[0], tlY = r[1];
    const float trX = r[2], trY = r[3];
    const float brX = r[4], brY = r[5];
    const float blX = r[6], blY = r[7];

    // Clockwise from just past the top-left corner.
    moveTo(l + tlX, t);
    lineTo(rt - trX, t);
    cubicTo(rt - trX * (1.0f - KAPPA), t,
            rt, t + trY * (1.0f - KAPPA),
            rt, t + trY);
    lineTo(rt, b - brY);
    cubicTo(rt, b - brY * (1.0f - KAPPA),
            rt - brX * (1.0f - KAPPA), b,
            rt - brX, b);
    lineTo(l + blX, b);
    cubicTo(l + blX * (1.0f - KAPPA), b,
            l, b - blY * (1.0f - KAPPA),
            l, b - blY);
    lineTo(l, t + tlY);
    cubicTo(l, t + tlY * (1.0f - KAPPA),
            l + tlX * (1.0f - KAPPA), t,
            l + tlX, t);
    close();
}

void Path::addOval(const RectF& rect) {
    const float w = rect.width();
    const float h = rect.height();
    if (w <= 0.0f || h <= 0.0f) return;

    const float rx = w * 0.5f;
    const float ry = h * 0.5f;
    const float cx = rect.left + rx;
    const float cy = rect.top + ry;
    const float ox = rx * KAPPA;
    const float oy = ry * KAPPA;

    // Clockwise from top centre.
    moveTo(cx, rect.top);
    cubicTo(cx + ox, rect.top, rect.right, cy - oy, rect.right, cy);
    cubicTo(rect.right, cy + oy, cx + ox, rect.bottom, cx, rect.bottom);
    cubicTo(cx - ox, rect.bottom, rect.left, cy + oy, rect.left, cy);
    cubicTo(rect.left, cy - oy, cx - ox, rect.top, cx, rect.top);
    close();
}

} // namespace graphics
} // namespace setu

#pragma once

#include <cstdint>
#include <vector>

#include "Rect.h"

namespace setu {
namespace graphics {

// A minimal android.graphics.Path: enough geometry for the drawable shapes real
// APKs actually use.
//
// Deliberately curve-only. Every arc is emitted as a cubic Bezier using the
// standard circle approximation (kappa), which keeps the verb set to four cases
// and means a backend only has to know how to move, line, curve and close. The
// approximation error is under 0.03% of the radius - far below a pixel for any
// corner or ring a layout will ask for.
//
// This is a plain value type: no GPU resources, cheap enough to copy into a
// display list alongside the Paint.
class Path {
public:
    // WINDING is Android's default. EVEN_ODD is what makes a ring a ring: two
    // ovals wound the same way, with the inner one punching a hole.
#ifdef WINDING
#undef WINDING
#endif
    enum class FillType : uint8_t {
        WINDING,
        EVEN_ODD
    };

    enum class Verb : uint8_t {
        MOVE_TO,  // 1 point
        LINE_TO,  // 1 point
        CUBIC_TO, // 3 points: control 1, control 2, end
        CLOSE     // 0 points
    };

    Path() = default;

    void reset();
    bool isEmpty() const { return mVerbs.empty(); }

    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void cubicTo(float x1, float y1, float x2, float y2, float x3, float y3);
    void close();

    void addRect(const RectF& rect);

    // Uniform corners.
    void addRoundRect(const RectF& rect, float rx, float ry);

    // Per-corner corners, in Android's order:
    //   [topLeftX, topLeftY, topRightX, topRightY,
    //    bottomRightX, bottomRightY, bottomLeftX, bottomLeftY]
    // Radii that would overlap along an edge are scaled down together, the way
    // Skia does, so an over-specified <corners> degrades instead of self-
    // intersecting.
    void addRoundRect(const RectF& rect, const float radii[8]);

    void addOval(const RectF& rect);

    void setFillType(FillType fillType) { mFillType = fillType; }
    FillType getFillType() const { return mFillType; }

    // Backends walk these together: each verb consumes 0, 1 or 3 points from the
    // front of the remaining point list.
    const std::vector<Verb>& getVerbs() const { return mVerbs; }
    const std::vector<float>& getPoints() const { return mPoints; }

private:
    void addPoint(float x, float y);

    std::vector<Verb> mVerbs;
    std::vector<float> mPoints; // x, y interleaved
    FillType mFillType = FillType::WINDING;
};

} // namespace graphics
} // namespace setu

#pragma once

namespace setu {
namespace view {

// Mirror of android.view.Gravity. The values are a bitfield, not an enum: a
// gravity of CENTER_HORIZONTAL|TOP (0x31) is a perfectly normal value that no
// amount of `gravity == CENTER` comparisons will ever match. Always mask with
// HORIZONTAL_GRAVITY_MASK / VERTICAL_GRAVITY_MASK before testing.
class Gravity {
public:
    static constexpr int NO_GRAVITY = 0x0000;

    static constexpr int AXIS_SPECIFIED = 0x0001;
    static constexpr int AXIS_PULL_BEFORE = 0x0002;
    static constexpr int AXIS_PULL_AFTER = 0x0004;
    static constexpr int AXIS_CLIP = 0x0008;

    static constexpr int AXIS_X_SHIFT = 0;
    static constexpr int AXIS_Y_SHIFT = 4;

    static constexpr int TOP = (AXIS_PULL_BEFORE | AXIS_SPECIFIED) << AXIS_Y_SHIFT;    // 0x30
    static constexpr int BOTTOM = (AXIS_PULL_AFTER | AXIS_SPECIFIED) << AXIS_Y_SHIFT;  // 0x50
    static constexpr int LEFT = (AXIS_PULL_BEFORE | AXIS_SPECIFIED) << AXIS_X_SHIFT;   // 0x03
    static constexpr int RIGHT = (AXIS_PULL_AFTER | AXIS_SPECIFIED) << AXIS_X_SHIFT;   // 0x05

    static constexpr int CENTER_VERTICAL = AXIS_SPECIFIED << AXIS_Y_SHIFT;             // 0x10
    static constexpr int FILL_VERTICAL = TOP | BOTTOM;                                 // 0x70
    static constexpr int CENTER_HORIZONTAL = AXIS_SPECIFIED << AXIS_X_SHIFT;           // 0x01
    static constexpr int FILL_HORIZONTAL = LEFT | RIGHT;                               // 0x07

    static constexpr int CENTER = CENTER_VERTICAL | CENTER_HORIZONTAL;                 // 0x11
    static constexpr int FILL = FILL_VERTICAL | FILL_HORIZONTAL;                       // 0x77

    static constexpr int CLIP_VERTICAL = AXIS_CLIP << AXIS_Y_SHIFT;                    // 0x80
    static constexpr int CLIP_HORIZONTAL = AXIS_CLIP << AXIS_X_SHIFT;                  // 0x08

    static constexpr int RELATIVE_LAYOUT_DIRECTION = 0x00800000;
    static constexpr int START = RELATIVE_LAYOUT_DIRECTION | LEFT;
    static constexpr int END = RELATIVE_LAYOUT_DIRECTION | RIGHT;

    static constexpr int HORIZONTAL_GRAVITY_MASK =
        (AXIS_SPECIFIED | AXIS_PULL_BEFORE | AXIS_PULL_AFTER) << AXIS_X_SHIFT;         // 0x07
    static constexpr int VERTICAL_GRAVITY_MASK =
        (AXIS_SPECIFIED | AXIS_PULL_BEFORE | AXIS_PULL_AFTER) << AXIS_Y_SHIFT;         // 0x70
    static constexpr int RELATIVE_HORIZONTAL_GRAVITY_MASK = START | END;

    static constexpr int DISPLAY_CLIP_VERTICAL = 0x10000000;
    static constexpr int DISPLAY_CLIP_HORIZONTAL = 0x01000000;

    // Resolves START/END against the layout direction. Windroid is LTR-only for
    // now, so RTL support is a matter of threading a real direction in here.
    static int getAbsoluteGravity(int gravity, bool rtl = false) {
        int result = gravity;
        if ((result & RELATIVE_LAYOUT_DIRECTION) != 0) {
            if ((result & START) == START) {
                result = (result & ~RELATIVE_HORIZONTAL_GRAVITY_MASK) | (rtl ? RIGHT : LEFT);
            } else if ((result & END) == END) {
                result = (result & ~RELATIVE_HORIZONTAL_GRAVITY_MASK) | (rtl ? LEFT : RIGHT);
            }
            result &= ~RELATIVE_LAYOUT_DIRECTION;
        }
        return result;
    }

    // Left edge for `contentWidth` placed inside [insetLeft, containerWidth - insetRight].
    static float applyHorizontal(int gravity, float containerWidth, float contentWidth,
                                 float insetLeft = 0.0f, float insetRight = 0.0f) {
        const float available = containerWidth - insetLeft - insetRight;
        switch (getAbsoluteGravity(gravity) & HORIZONTAL_GRAVITY_MASK) {
            case RIGHT:
                return insetLeft + (available - contentWidth);
            case CENTER_HORIZONTAL:
            case FILL_HORIZONTAL:  // nothing to stretch here; centre like AOSP text does
                return insetLeft + (available - contentWidth) / 2.0f;
            case LEFT:
            default:
                return insetLeft;
        }
    }

    // Top edge for `contentHeight` placed inside [insetTop, containerHeight - insetBottom].
    static float applyVertical(int gravity, float containerHeight, float contentHeight,
                               float insetTop = 0.0f, float insetBottom = 0.0f) {
        const float available = containerHeight - insetTop - insetBottom;
        switch (gravity & VERTICAL_GRAVITY_MASK) {
            case BOTTOM:
                return insetTop + (available - contentHeight);
            case CENTER_VERTICAL:
            case FILL_VERTICAL:
                return insetTop + (available - contentHeight) / 2.0f;
            case TOP:
            default:
                return insetTop;
        }
    }
};

} // namespace view
} // namespace setu

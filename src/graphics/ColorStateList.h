#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace setu {
namespace graphics {

// android.content.res.ColorStateList: maps View state sets to colours.
//
// Deliberately *not* a Drawable and deliberately not under drawable/. In AOSP
// this lives in android.content.res and is a plain value: a text colour, a
// <shape>'s solid fill and a tint are all "a colour that depends on state", and
// none of them wants a drawable's bounds, alpha or callback machinery. Keeping
// it a value means TextView can hold one without holding a second Drawable.
//
//   <selector xmlns:android="...">
//     <item android:state_pressed="true"  android:color="#ff0000"/>
//     <item android:state_enabled="false" android:color="#80000000"/>
//     <item android:color="#000000"/>
//   </selector>
//
// becomes three (spec, colour) pairs, matched top to bottom by the same
// StateSet::matches() a <selector> drawable uses - so a colour selector and a
// drawable selector can never disagree about what "pressed and disabled" means.
//
// Immutable once constructed. withAlpha() returns a new list rather than
// mutating, which is what lets a resolved instance be shared between views.
class ColorStateList {
public:
    // The colour reported when a list carries no items at all. AOSP uses
    // Color.RED on purpose: it is a loud failure marker, not a neutral
    // fallback. A screen that comes up unexpectedly red is a malformed colour
    // resource, and that is worth being obvious.
    static constexpr uint32_t DEFAULT_COLOR = 0xFFFF0000;

    // Per-item colour when an <item> omits android:color. Magenta, for the same
    // reason as above.
    static constexpr uint32_t MISSING_ITEM_COLOR = 0xFFFF00FF;

    ColorStateList() { onColorsChanged(); }

    // Parallel arrays, one colour per spec, in document order. Sizes are
    // reconciled on entry so a malformed inflate cannot produce a list that
    // indexes out of range later.
    ColorStateList(std::vector<std::vector<int>> specs, std::vector<uint32_t> colors);

    // A list of exactly one colour, matching every state. AOSP's EMPTY is
    // { new int[0] } - one *wildcard* spec, not zero specs, which is what makes
    // valueOf() non-stateful and its default the colour itself.
    static std::shared_ptr<ColorStateList> valueOf(uint32_t color);

    // True when the colour actually depends on state. AOSP's rule is the first
    // spec being non-empty, not the item count: a list whose first item is the
    // wildcard can never reach its later items, so it is a constant colour
    // however many items follow it.
    bool isStateful() const { return !mSpecs.empty() && !mSpecs[0].empty(); }

    // Every colour in the list has alpha 0xFF, so a fill using it needs no
    // blending whatever state it lands on.
    bool isOpaque() const { return mIsOpaque; }

    // No items at all - the result of inflating a <selector> with no usable
    // children. Distinct from "not a colour state list", which the inflater
    // reports as a null pointer.
    bool isEmpty() const { return mSpecs.empty(); }

    // The first item whose spec matches, or `defaultColor` when none does.
    // Order is document order, so more specific items must come first - exactly
    // the rule that applies to a <selector> drawable.
    uint32_t getColorForState(const std::vector<int>& stateSet, uint32_t defaultColor) const;

    // The colour for a view that has not reported any state, used when a caller
    // has no state vector to offer. This is the last wildcard item, not the
    // first item - see onColorsChanged().
    uint32_t getDefaultColor() const { return mDefaultColor; }

    // The same states and colours with every alpha channel replaced. Used for
    // a View-level alpha applied on top of an authored colour.
    std::shared_ptr<ColorStateList> withAlpha(int alpha) const;

    // Whether any spec mentions `state`, in either polarity. Wildcards do not
    // count. Lets a caller skip re-resolving on a state change that this list
    // demonstrably ignores.
    bool hasState(int state) const;

    const std::vector<std::vector<int>>& getStates() const { return mSpecs; }
    const std::vector<uint32_t>& getColors() const { return mColors; }

    // One line for a log: "ColorStateList{[state_pressed]=#ffff0000, []=#ff000000}".
    std::string describe() const;

private:
    // Recomputes mDefaultColor and mIsOpaque. Called once from every
    // constructor; the list is immutable afterwards.
    void onColorsChanged();

    std::vector<std::vector<int>> mSpecs;
    std::vector<uint32_t> mColors;
    uint32_t mDefaultColor = DEFAULT_COLOR;
    bool mIsOpaque = true;
};

using ColorStateListPtr = std::shared_ptr<ColorStateList>;

} // namespace graphics
} // namespace setu

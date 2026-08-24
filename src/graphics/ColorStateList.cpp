#include "ColorStateList.h"

#include <cstdio>

#include "drawable/StateSet.h"

namespace setu {
namespace graphics {

ColorStateList::ColorStateList(std::vector<std::vector<int>> specs,
                               std::vector<uint32_t> colors)
    : mSpecs(std::move(specs)), mColors(std::move(colors)) {
    // A malformed inflate could hand us mismatched arrays; AOSP trusts its
    // parser to keep them equal, but a stray colour with no spec (or the
    // reverse) would index out of range in getColorForState. Trim to the
    // shorter of the two so every index is always valid.
    if (mColors.size() != mSpecs.size()) {
        const size_t n = mSpecs.size() < mColors.size() ? mSpecs.size() : mColors.size();
        mSpecs.resize(n);
        mColors.resize(n);
    }
    onColorsChanged();
}

std::shared_ptr<ColorStateList> ColorStateList::valueOf(uint32_t color) {
    // EMPTY = { new int[0] }: a single wildcard spec paired with the colour.
    // One item, empty spec - so isStateful() is false and the colour matches
    // every state.
    return std::make_shared<ColorStateList>(
        std::vector<std::vector<int>>{ std::vector<int>{} },
        std::vector<uint32_t>{ color });
}

uint32_t ColorStateList::getColorForState(const std::vector<int>& stateSet,
                                          uint32_t defaultColor) const {
    for (size_t i = 0; i < mSpecs.size(); ++i) {
        if (StateSet::matches(mSpecs[i], stateSet)) {
            return mColors[i];
        }
    }
    return defaultColor;
}

std::shared_ptr<ColorStateList> ColorStateList::withAlpha(int alpha) const {
    std::vector<uint32_t> colors(mColors.size());
    const uint32_t a = (uint32_t)(alpha & 0xFF) << 24;
    for (size_t i = 0; i < mColors.size(); ++i) {
        colors[i] = (mColors[i] & 0x00FFFFFF) | a;
    }
    return std::make_shared<ColorStateList>(mSpecs, std::move(colors));
}

bool ColorStateList::hasState(int state) const {
    for (const auto& spec : mSpecs) {
        for (int s : spec) {
            // Either polarity counts: a spec entry is +state when the state must
            // be set and -state when it must be absent, and both are references.
            if (s == state || s == -state) return true;
        }
    }
    return false;
}

std::string ColorStateList::describe() const {
    std::string out = "ColorStateList{";
    char hex[16];
    for (size_t i = 0; i < mSpecs.size(); ++i) {
        if (i > 0) out += ", ";
        out += StateSet::describe(mSpecs[i]);
        out += '=';
        std::snprintf(hex, sizeof(hex), "#%08x", mColors[i]);
        out += hex;
    }
    out += '}';
    return out;
}

void ColorStateList::onColorsChanged() {
    uint32_t defaultColor = DEFAULT_COLOR;
    bool isOpaque = true;

    const size_t n = mSpecs.size();
    if (n > 0) {
        // Provisionally the first colour, then the *last* wildcard item wins.
        // AOSP scans backwards from the end down to (but not including) index 0
        // and takes the first empty spec it meets - which is the last empty spec
        // in document order. Index 0 is excluded because it is already the
        // provisional default; a wildcard there means the list is a constant and
        // colors[0] is correct anyway.
        defaultColor = mColors[0];
        for (size_t i = n - 1; i > 0; --i) {
            if (mSpecs[i].empty()) {
                defaultColor = mColors[i];
                break;
            }
        }
        for (size_t i = 0; i < n; ++i) {
            if ((mColors[i] >> 24) != 0xFF) {
                isOpaque = false;
                break;
            }
        }
    }

    mDefaultColor = defaultColor;
    mIsOpaque = isOpaque;
}

} // namespace graphics
} // namespace setu

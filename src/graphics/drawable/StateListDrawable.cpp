#include "StateListDrawable.h"

#include "StateSet.h"

namespace setu {
namespace graphics {

namespace {

// Returned by getStateSet() for an index that does not exist, so callers do not
// have to check before dereferencing.
const std::vector<int>& emptyStateSet() {
    static const std::vector<int> empty;
    return empty;
}

} // namespace

void StateListDrawable::addState(const std::vector<int>& stateSet, DrawablePtr drawable) {
    if (!drawable) return;

    const int pos = addChild(std::move(drawable));
    if (pos < 0) return;

    if ((int)mStateSets.size() <= pos) {
        mStateSets.resize((size_t)pos + 1);
    }
    mStateSets[(size_t)pos] = stateSet;

    // The item just added may be the one the current state wants. AOSP re-runs the
    // lookup here for the same reason, which is what makes addState() usable after
    // the drawable is already installed on a View.
    onStateChange(getState());
}

const std::vector<int>& StateListDrawable::getStateSet(int index) const {
    if (index < 0 || index >= (int)mStateSets.size()) return emptyStateSet();
    return mStateSets[(size_t)index];
}

int StateListDrawable::indexOfStateSet(const std::vector<int>& stateSet) const {
    const int count = getChildCount();
    for (int i = 0; i < count && i < (int)mStateSets.size(); ++i) {
        if (StateSet::matches(mStateSets[(size_t)i], stateSet)) return i;
    }
    return -1;
}

bool StateListDrawable::onStateChange(const std::vector<int>& stateSet) {
    // Forwarded first so the child currently on screen learns the new state even
    // if it turns out to still be the right child.
    const bool changed = DrawableContainer::onStateChange(stateSet);

    int index = indexOfStateSet(stateSet);
    if (index < 0) {
        // Nothing matched. AOSP retries against the empty state set, which is not
        // quite "find the wildcard item": an item asking only for absences
        // (state_enabled="false" and nothing else) also matches an empty state and
        // can win the retry. Kept as-is, because that is the item a real device
        // would land on too.
        index = indexOfStateSet(emptyStateSet());
    }

    return selectDrawable(index) || changed;
}

} // namespace graphics
} // namespace setu

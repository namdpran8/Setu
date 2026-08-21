#include "ConstraintAnchor.h"
#include "ConstraintWidget.h"
#include "SolverVariable.h"
#include "Cache.h"
// Note: You will need to implement/include WidgetGroup, Grouping, and Guideline.
// #include "analyzer/WidgetGroup.h"
// #include "analyzer/Grouping.h"
// #include "Guideline.h"

namespace setu::cassowary {

// 145: ConstraintAnchor(ConstraintWidget owner, Type type)
ConstraintAnchor::ConstraintAnchor(ConstraintWidget* owner, Type type)
    : mOwner(owner), mType(type) {}

// 41: findDependents(int orientation, ArrayList<WidgetGroup> list, WidgetGroup group)
void ConstraintAnchor::findDependents(int orientation, std::vector<WidgetGroup*>& list, WidgetGroup* group) {
    if (!mDependents.empty()) {
        for (ConstraintAnchor* anchor : mDependents) {
            // Grouping::findDependents(anchor->mOwner, orientation, list, group);
            // TODO: Uncomment once Grouping is ported
        }
    }
}

// 49: getDependents()
std::unordered_set<ConstraintAnchor*> ConstraintAnchor::getDependents() {
    return mDependents;
}

// 54: hasDependents()
bool ConstraintAnchor::hasDependents() const {
    return !mDependents.empty();
}

// 62: hasCenteredDependents()
bool ConstraintAnchor::hasCenteredDependents() const {
    for (ConstraintAnchor* anchor : mDependents) {
        ConstraintAnchor* opposite = anchor->getOpposite();
        if (opposite && opposite->isConnected()) {
            return true;
        }
    }
    return false;
}

// 76: setFinalValue(int finalValue)
void ConstraintAnchor::setFinalValue(int finalValue) {
    mFinalValue = finalValue;
    mHasFinalValue = true;
}

// 82: getFinalValue()
int ConstraintAnchor::getFinalValue() const {
    if (!mHasFinalValue) {
        return 0;
    }
    return mFinalValue;
}

// 90: resetFinalResolution()
void ConstraintAnchor::resetFinalResolution() {
    mHasFinalValue = false;
    mFinalValue = 0;
}

// 96: hasFinalValue()
bool ConstraintAnchor::hasFinalValue() const {
    return mHasFinalValue;
}

// 116: copyFrom(ConstraintAnchor source, HashMap<ConstraintWidget, ConstraintWidget> map)
void ConstraintAnchor::copyFrom(ConstraintAnchor* source, std::unordered_map<ConstraintWidget*, ConstraintWidget*>& map) {
    if (mTarget != nullptr) {
        mTarget->mDependents.erase(this);
    }
    if (source->mTarget != nullptr) {
        Type type = source->mTarget->getType();
        ConstraintWidget* owner = map[source->mTarget->mOwner];
        mTarget = owner->getAnchor(type);
    } else {
        mTarget = nullptr;
    }
    if (mTarget != nullptr) {
        mTarget->mDependents.insert(this);
    }
    mMargin = source->mMargin;
    mGoneMargin = source->mGoneMargin;
}

// 153: getSolverVariable()
SolverVariable* ConstraintAnchor::getSolverVariable() {
    return mSolverVariable;
}

// 160: resetSolverVariable(Cache cache)
void ConstraintAnchor::resetSolverVariable(Cache* cache) {
    if (mSolverVariable == nullptr) {
        mSolverVariable = new SolverVariable(SolverVariable::Type::UNRESTRICTED, ""); // string null to ""
    } else {
        mSolverVariable->reset();
    }
}

// 173: getOwner()
ConstraintWidget* ConstraintAnchor::getOwner() const {
    return mOwner;
}

// 182: getType()
ConstraintAnchor::Type ConstraintAnchor::getType() const {
    return mType;
}

// 191: getMargin()
int ConstraintAnchor::getMargin() const {
    if (mOwner->getVisibility() == 8 /* ConstraintWidget::GONE */) { // using 8 for GONE
        return 0;
    }
    if (mGoneMargin != UNSET_GONE_MARGIN && mTarget != nullptr
            && mTarget->mOwner->getVisibility() == 8 /* ConstraintWidget::GONE */) {
        return mGoneMargin;
    }
    return mMargin;
}

// 207: getTarget()
ConstraintAnchor* ConstraintAnchor::getTarget() const {
    return mTarget;
}

// 214: reset()
void ConstraintAnchor::reset() {
    if (mTarget != nullptr) {
        mTarget->mDependents.erase(this);
    }
    mDependents.clear();
    mTarget = nullptr;
    mMargin = 0;
    mGoneMargin = UNSET_GONE_MARGIN;
    mHasFinalValue = false;
    mFinalValue = 0;
}

// 234: connect(ConstraintAnchor toAnchor, int margin, int goneMargin, boolean forceConnection)
bool ConstraintAnchor::connect(ConstraintAnchor* toAnchor, int margin, int goneMargin, bool forceConnection) {
    if (toAnchor == nullptr) {
        reset();
        return true;
    }
    if (!forceConnection && !isValidConnection(toAnchor)) {
        return false;
    }
    mTarget = toAnchor;
    mTarget->mDependents.insert(this);
    mMargin = margin;
    mGoneMargin = goneMargin;
    return true;
}

// 261: connect(ConstraintAnchor toAnchor, int margin)
bool ConstraintAnchor::connect(ConstraintAnchor* toAnchor, int margin) {
    return connect(toAnchor, margin, UNSET_GONE_MARGIN, false);
}

// 270: isConnected()
bool ConstraintAnchor::isConnected() const {
    return mTarget != nullptr;
}

// 280: isValidConnection(ConstraintAnchor anchor)
bool ConstraintAnchor::isValidConnection(ConstraintAnchor* anchor) const {
    if (anchor == nullptr) {
        return false;
    }
    Type target = anchor->getType();
    if (target == mType) {
        if (mType == Type::BASELINE
                && (!anchor->getOwner()->hasBaseline() || !getOwner()->hasBaseline())) {
            return false;
        }
        return true;
    }
    switch (mType) {
        case Type::CENTER: {
            return target != Type::BASELINE && target != Type::CENTER_X
                    && target != Type::CENTER_Y;
        }
        case Type::LEFT:
        case Type::RIGHT: {
            bool isCompatible = target == Type::LEFT || target == Type::RIGHT;
            // TODO: instance of Guideline
            // if (dynamic_cast<Guideline*>(anchor->getOwner())) {
            //     isCompatible = isCompatible || target == Type::CENTER_X;
            // }
            return isCompatible;
        }
        case Type::TOP:
        case Type::BOTTOM: {
            bool isCompatible = target == Type::TOP || target == Type::BOTTOM;
            // TODO: instance of Guideline
            // if (dynamic_cast<Guideline*>(anchor->getOwner())) {
            //     isCompatible = isCompatible || target == Type::CENTER_Y;
            // }
            return isCompatible;
        }
        case Type::BASELINE: {
            if (target == Type::LEFT || target == Type::RIGHT) {
                return false;
            }
            return true;
        }
        case Type::CENTER_X:
        case Type::CENTER_Y:
        case Type::NONE:
            return false;
    }
    assert(false && "isValidConnection unhandled type");
    return false;
}

// 333: isSideAnchor()
bool ConstraintAnchor::isSideAnchor() const {
    switch (mType) {
        case Type::LEFT:
        case Type::RIGHT:
        case Type::TOP:
        case Type::BOTTOM:
            return true;
        case Type::BASELINE:
        case Type::CENTER:
        case Type::CENTER_X:
        case Type::CENTER_Y:
        case Type::NONE:
            return false;
    }
    assert(false && "isSideAnchor unhandled type");
    return false;
}

// 357: isSimilarDimensionConnection(ConstraintAnchor anchor)
bool ConstraintAnchor::isSimilarDimensionConnection(ConstraintAnchor* anchor) const {
    Type target = anchor->getType();
    if (target == mType) {
        return true;
    }
    switch (mType) {
        case Type::CENTER: {
            return target != Type::BASELINE;
        }
        case Type::LEFT:
        case Type::RIGHT:
        case Type::CENTER_X: {
            return target == Type::LEFT || target == Type::RIGHT || target == Type::CENTER_X;
        }
        case Type::TOP:
        case Type::BOTTOM:
        case Type::CENTER_Y:
        case Type::BASELINE: {
            return target == Type::TOP || target == Type::BOTTOM
                    || target == Type::CENTER_Y || target == Type::BASELINE;
        }
        case Type::NONE:
            return false;
    }
    assert(false && "isSimilarDimensionConnection unhandled type");
    return false;
}

// 389: setMargin(int margin)
void ConstraintAnchor::setMargin(int margin) {
    if (isConnected()) {
        mMargin = margin;
    }
}

// 400: setGoneMargin(int margin)
void ConstraintAnchor::setGoneMargin(int margin) {
    if (isConnected()) {
        mGoneMargin = margin;
    }
}

// 411: isVerticalAnchor()
bool ConstraintAnchor::isVerticalAnchor() const {
    switch (mType) {
        case Type::LEFT:
        case Type::RIGHT:
        case Type::CENTER:
        case Type::CENTER_X:
            return false;
        case Type::CENTER_Y:
        case Type::TOP:
        case Type::BOTTOM:
        case Type::BASELINE:
        case Type::NONE:
            return true;
    }
    assert(false && "isVerticalAnchor unhandled type");
    return false;
}

// 433: toString()
std::string ConstraintAnchor::toString() const {
    // return mOwner->getDebugName() + ":" + std::to_string(static_cast<int>(mType));
    return "ConstraintAnchor:" + std::to_string(static_cast<int>(mType));
}

// 448: isConnectionAllowed(ConstraintWidget target, ConstraintAnchor anchor)
bool ConstraintAnchor::isConnectionAllowed(ConstraintWidget* target, ConstraintAnchor* anchor) {
    if (ALLOW_BINARY) {
        if (anchor != nullptr && anchor->getTarget() == this) {
            return true;
        }
    }
    return isConnectionAllowed(target);
}

// 466: isConnectionAllowed(ConstraintWidget target)
bool ConstraintAnchor::isConnectionAllowed(ConstraintWidget* target) {
    std::unordered_set<ConstraintWidget*> checked;
    if (isConnectionToMe(target, checked)) {
        return false;
    }
    ConstraintWidget* parent = getOwner()->getParent();
    if (parent == target) {
        return true;
    }
    if (target->getParent() == parent) {
        return true;
    }
    return false;
}

// 487: isConnectionToMe(ConstraintWidget target, HashSet<ConstraintWidget> checked)
bool ConstraintAnchor::isConnectionToMe(ConstraintWidget* target, std::unordered_set<ConstraintWidget*>& checked) {
    if (checked.count(target)) {
        return false;
    }
    checked.insert(target);

    if (target == getOwner()) {
        return true;
    }
    std::vector<ConstraintAnchor*> targetAnchors = target->getAnchors();
    for (size_t i = 0, targetAnchorsSize = targetAnchors.size(); i < targetAnchorsSize; i++) {
        ConstraintAnchor* anchor = targetAnchors[i];
        if (anchor->isSimilarDimensionConnection(this) && anchor->isConnected()) {
            if (isConnectionToMe(anchor->getTarget()->getOwner(), checked)) {
                return true;
            }
        }
    }
    return false;
}

// 513: getOpposite()
ConstraintAnchor* ConstraintAnchor::getOpposite() const {
    switch (mType) {
        case Type::LEFT: return &mOwner->mRight;
        case Type::RIGHT: return &mOwner->mLeft;
        case Type::TOP: return &mOwner->mBottom;
        case Type::BOTTOM: return &mOwner->mTop;
        case Type::BASELINE:
        case Type::CENTER:
        case Type::CENTER_X:
        case Type::CENTER_Y:
        case Type::NONE:
            return nullptr;
    }
    assert(false && "getOpposite unhandled type");
    return nullptr;
}

} // namespace setu::cassowary

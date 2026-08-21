#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cassert>

namespace setu::cassowary {

class ConstraintWidget;
class SolverVariable;
class Cache;
class WidgetGroup;

class ConstraintAnchor {
public:
    enum class Type { NONE, LEFT, TOP, RIGHT, BOTTOM, BASELINE, CENTER, CENTER_X, CENTER_Y };

private:
    static constexpr bool ALLOW_BINARY = false;
    std::unordered_set<ConstraintAnchor*> mDependents;
    int mFinalValue = 0;
    bool mHasFinalValue = false;

    static constexpr int UNSET_GONE_MARGIN = -2147483648; // Integer.MIN_VALUE

public:
    ConstraintWidget* const mOwner;
    const Type mType;
    ConstraintAnchor* mTarget = nullptr;
    int mMargin = 0;
    int mGoneMargin = UNSET_GONE_MARGIN;

    SolverVariable* mSolverVariable = nullptr;

    ConstraintAnchor(ConstraintWidget* owner, Type type);

    void findDependents(int orientation, std::vector<WidgetGroup*>& list, WidgetGroup* group);
    
    std::unordered_set<ConstraintAnchor*> getDependents();
    
    bool hasDependents() const;
    bool hasCenteredDependents() const;
    
    void setFinalValue(int finalValue);
    int getFinalValue() const;
    void resetFinalResolution();
    bool hasFinalValue() const;

    void copyFrom(ConstraintAnchor* source, std::unordered_map<ConstraintWidget*, ConstraintWidget*>& map);

    SolverVariable* getSolverVariable();
    void resetSolverVariable(Cache* cache);

    ConstraintWidget* getOwner() const;
    Type getType() const;
    int getMargin() const;
    ConstraintAnchor* getTarget() const;
    
    void reset();

    bool connect(ConstraintAnchor* toAnchor, int margin, int goneMargin, bool forceConnection);
    bool connect(ConstraintAnchor* toAnchor, int margin);

    bool isConnected() const;
    bool isValidConnection(ConstraintAnchor* anchor) const;
    bool isSideAnchor() const;
    bool isSimilarDimensionConnection(ConstraintAnchor* anchor) const;
    
    void setMargin(int margin);
    void setGoneMargin(int margin);
    bool isVerticalAnchor() const;
    
    std::string toString() const;

    bool isConnectionAllowed(ConstraintWidget* target, ConstraintAnchor* anchor);
    bool isConnectionAllowed(ConstraintWidget* target);
    
    ConstraintAnchor* getOpposite() const;

private:
    bool isConnectionToMe(ConstraintWidget* target, std::unordered_set<ConstraintWidget*>& checked);
};

} // namespace setu::cassowary

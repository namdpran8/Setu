#pragma once

#include "ConstraintWidget.h"
#include "ConstraintAnchor.h"

namespace setu::cassowary {

class Guideline : public ConstraintWidget {
public:
    static constexpr int HORIZONTAL = 0;
    static constexpr int VERTICAL = 1;

    static constexpr int RELATIVE_PERCENT = 0;
    static constexpr int RELATIVE_BEGIN = 1;
    static constexpr int RELATIVE_END = 2;
    static constexpr int RELATIVE_UNKNOWN = -1;

protected:
    float mRelativePercent = -1.0f;
    int mRelativeBegin = -1;
    int mRelativeEnd = -1;
    bool mResolved = false;

private:
    ConstraintAnchor* mAnchor;
    int mOrientation = HORIZONTAL;
    int mMinimumPosition = 0;

public:
    Guideline();
    virtual ~Guideline() = default;

    virtual bool allowedInBarrier() const;
    
    int getRelativeBehaviour() const;
    void setOrientation(int orientation);
    int getOrientation() const;
    void setMinimumPosition(int minimum);
    int getMinimumPosition() const;

    virtual ConstraintAnchor* getAnchor(ConstraintAnchor::Type anchorType);
    virtual ConstraintAnchor* getAnchor(int anchorType) { 
        return getAnchor(static_cast<ConstraintAnchor::Type>(anchorType)); 
    }

    std::string getType() const override;

    void setGuideBegin(int value);
    int getRelativeBegin() const;
    void setGuideEnd(int value);
    int getRelativeEnd() const;
    void setGuidePercent(float value);
    float getRelativePercent() const;

    virtual void addToSolver(LinearSystem* system, bool optimize);
    virtual void updateFromSolver(LinearSystem* system, bool optimize);

    ConstraintAnchor* getAnchor() {
        return mAnchor;
    }
};

} // namespace setu::cassowary

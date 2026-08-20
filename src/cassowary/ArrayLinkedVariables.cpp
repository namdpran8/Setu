// ArrayLinkedVariables.cpp — Ported from ArrayLinkedVariables.java:1-702
// Array-based linked list storing variable coefficients per row.
//
// Copyright (C) 2016 The Android Open Source Project — Apache 2.0

#include "ArrayLinkedVariables.h"
#include "Cache.h"

#include <iostream>

namespace setu::cassowary {

/// ArrayLinkedVariables.java:110 — constructor
ArrayLinkedVariables::ArrayLinkedVariables(ArrayRow* arrayRow, Cache* cache)
    : mRow(arrayRow), mCache(cache) {
    // Pre-allocate initial capacity to avoid reallocation during first pivots.
    // Matches Java's initial mRowSize = 8.
    mArrayIndices.resize(mRowSize, 0);
    mArrayNextIndices.resize(mRowSize, 0);
    mArrayValues.resize(mRowSize, 0.0f);
}

/// ArrayLinkedVariables.java:127 — public final void put(SolverVariable variable, float value)
void ArrayLinkedVariables::put(SolverVariable* variable, float value) {
    if (value == 0) {
        remove(variable, true);
        return;
    }
    // Special casing empty list...
    if (mHead == NONE) {
        mHead = 0;
        mArrayValues[mHead] = value;
        mArrayIndices[mHead] = variable->id;
        mArrayNextIndices[mHead] = NONE;
        variable->usageInRowCount++;
        variable->addToRow(mRow);
        mCurrentSize++;
        if (!mDidFillOnce) {
            mLast++;
            if (mLast >= static_cast<int>(mArrayIndices.size())) {
                mDidFillOnce = true;
                mLast = static_cast<int>(mArrayIndices.size()) - 1;
            }
        }
        return;
    }
    int current = mHead;
    int previous = NONE;
    int counter = 0;
    while (current != NONE && counter < mCurrentSize) {
        if (mArrayIndices[current] == variable->id) {
            mArrayValues[current] = value;
            return;
        }
        if (mArrayIndices[current] < variable->id) {
            previous = current;
        }
        current = mArrayNextIndices[current];
        counter++;
    }

    // Not found, we need to insert
    int availableIndice = mLast + 1;
    if (mDidFillOnce) {
        if (mArrayIndices[mLast] == NONE) {
            availableIndice = mLast;
        } else {
            availableIndice = static_cast<int>(mArrayIndices.size());
        }
    }
    if (availableIndice >= static_cast<int>(mArrayIndices.size())) {
        if (mCurrentSize < static_cast<int>(mArrayIndices.size())) {
            for (int i = 0; i < static_cast<int>(mArrayIndices.size()); i++) {
                if (mArrayIndices[i] == NONE) {
                    availableIndice = i;
                    break;
                }
            }
        }
    }
    // Grow arrays as needed — Java: Arrays.copyOf(..., mRowSize)
    if (availableIndice >= static_cast<int>(mArrayIndices.size())) {
        availableIndice = static_cast<int>(mArrayIndices.size());
        mRowSize *= 2;
        mDidFillOnce = false;
        mLast = availableIndice - 1;
        mArrayValues.resize(mRowSize, 0.0f);
        mArrayIndices.resize(mRowSize, 0);
        mArrayNextIndices.resize(mRowSize, 0);
    }

    // Insert the element
    mArrayIndices[availableIndice] = variable->id;
    mArrayValues[availableIndice] = value;
    if (previous != NONE) {
        mArrayNextIndices[availableIndice] = mArrayNextIndices[previous];
        mArrayNextIndices[previous] = availableIndice;
    } else {
        mArrayNextIndices[availableIndice] = mHead;
        mHead = availableIndice;
    }
    variable->usageInRowCount++;
    variable->addToRow(mRow);
    mCurrentSize++;
    if (!mDidFillOnce) {
        mLast++;
    }
    if (mCurrentSize >= static_cast<int>(mArrayIndices.size())) {
        mDidFillOnce = true;
    }
    if (mLast >= static_cast<int>(mArrayIndices.size())) {
        mDidFillOnce = true;
        mLast = static_cast<int>(mArrayIndices.size()) - 1;
    }
}

/// ArrayLinkedVariables.java:237 — public void add(SolverVariable variable, float value, boolean removeFromDefinition)
void ArrayLinkedVariables::add(SolverVariable* variable, float value, bool removeFromDefinition) {
    if (value > -sEpsilon && value < sEpsilon) {
        return;
    }
    // Special casing empty list...
    if (mHead == NONE) {
        mHead = 0;
        mArrayValues[mHead] = value;
        mArrayIndices[mHead] = variable->id;
        mArrayNextIndices[mHead] = NONE;
        variable->usageInRowCount++;
        variable->addToRow(mRow);
        mCurrentSize++;
        if (!mDidFillOnce) {
            mLast++;
            if (mLast >= static_cast<int>(mArrayIndices.size())) {
                mDidFillOnce = true;
                mLast = static_cast<int>(mArrayIndices.size()) - 1;
            }
        }
        return;
    }
    int current = mHead;
    int previous = NONE;
    int counter = 0;
    while (current != NONE && counter < mCurrentSize) {
        int idx = mArrayIndices[current];
        if (idx == variable->id) {
            float v = mArrayValues[current] + value;
            if (v > -sEpsilon && v < sEpsilon) {
                v = 0;
            }
            mArrayValues[current] = v;
            // Possibly delete immediately
            if (v == 0) {
                if (current == mHead) {
                    mHead = mArrayNextIndices[current];
                } else {
                    mArrayNextIndices[previous] = mArrayNextIndices[current];
                }
                if (removeFromDefinition) {
                    variable->removeFromRow(mRow);
                }
                if (mDidFillOnce) {
                    mLast = current;
                }
                variable->usageInRowCount--;
                mCurrentSize--;
            }
            return;
        }
        if (mArrayIndices[current] < variable->id) {
            previous = current;
        }
        current = mArrayNextIndices[current];
        counter++;
    }

    // Not found, we need to insert
    int availableIndice = mLast + 1;
    if (mDidFillOnce) {
        if (mArrayIndices[mLast] == NONE) {
            availableIndice = mLast;
        } else {
            availableIndice = static_cast<int>(mArrayIndices.size());
        }
    }
    if (availableIndice >= static_cast<int>(mArrayIndices.size())) {
        if (mCurrentSize < static_cast<int>(mArrayIndices.size())) {
            for (int i = 0; i < static_cast<int>(mArrayIndices.size()); i++) {
                if (mArrayIndices[i] == NONE) {
                    availableIndice = i;
                    break;
                }
            }
        }
    }
    if (availableIndice >= static_cast<int>(mArrayIndices.size())) {
        availableIndice = static_cast<int>(mArrayIndices.size());
        mRowSize *= 2;
        mDidFillOnce = false;
        mLast = availableIndice - 1;
        mArrayValues.resize(mRowSize, 0.0f);
        mArrayIndices.resize(mRowSize, 0);
        mArrayNextIndices.resize(mRowSize, 0);
    }

    mArrayIndices[availableIndice] = variable->id;
    mArrayValues[availableIndice] = value;
    if (previous != NONE) {
        mArrayNextIndices[availableIndice] = mArrayNextIndices[previous];
        mArrayNextIndices[previous] = availableIndice;
    } else {
        mArrayNextIndices[availableIndice] = mHead;
        mHead = availableIndice;
    }
    variable->usageInRowCount++;
    variable->addToRow(mRow);
    mCurrentSize++;
    if (!mDidFillOnce) {
        mLast++;
    }
    if (mLast >= static_cast<int>(mArrayIndices.size())) {
        mDidFillOnce = true;
        mLast = static_cast<int>(mArrayIndices.size()) - 1;
    }
}

/// ArrayLinkedVariables.java:361 — public float use(ArrayRow definition, boolean removeFromDefinition)
float ArrayLinkedVariables::use(ArrayRow* definition, bool removeFromDefinition) {
    float value = get(definition->mVariable);
    remove(definition->mVariable, removeFromDefinition);
    ArrayRowVariables* definitionVariables = definition->variables.get();
    int definitionSize = definitionVariables->getCurrentSize();
    for (int i = 0; i < definitionSize; i++) {
        SolverVariable* definitionVariable = definitionVariables->getVariable(i);
        float definitionValue = definitionVariables->get(definitionVariable);
        this->add(definitionVariable, definitionValue * value, removeFromDefinition);
    }
    return value;
}

/// ArrayLinkedVariables.java:381 — public final float remove(SolverVariable variable, boolean removeFromDefinition)
float ArrayLinkedVariables::remove(SolverVariable* variable, bool removeFromDefinition) {
    if (mCandidate == variable) {
        mCandidate = nullptr;
    }
    if (mHead == NONE) {
        return 0;
    }
    int current = mHead;
    int previous = NONE;
    int counter = 0;
    while (current != NONE && counter < mCurrentSize) {
        int idx = mArrayIndices[current];
        if (idx == variable->id) {
            if (current == mHead) {
                mHead = mArrayNextIndices[current];
            } else {
                mArrayNextIndices[previous] = mArrayNextIndices[current];
            }
            if (removeFromDefinition) {
                variable->removeFromRow(mRow);
            }
            variable->usageInRowCount--;
            mCurrentSize--;
            mArrayIndices[current] = NONE;
            if (mDidFillOnce) {
                mLast = current;
            }
            return mArrayValues[current];
        }
        previous = current;
        current = mArrayNextIndices[current];
        counter++;
    }
    return 0;
}

/// ArrayLinkedVariables.java:423 — public final void clear()
void ArrayLinkedVariables::clear() {
    int current = mHead;
    int counter = 0;
    while (current != NONE && counter < mCurrentSize) {
        SolverVariable* variable = mCache->mIndexedVariables[mArrayIndices[current]];
        if (variable != nullptr) {
            variable->removeFromRow(mRow);
        }
        current = mArrayNextIndices[current];
        counter++;
    }
    mHead = NONE;
    mLast = NONE;
    mDidFillOnce = false;
    mCurrentSize = 0;
}

/// ArrayLinkedVariables.java:448 — public boolean contains(SolverVariable variable)
bool ArrayLinkedVariables::contains(SolverVariable* variable) {
    if (mHead == NONE) {
        return false;
    }
    int current = mHead;
    int counter = 0;
    while (current != NONE && counter < mCurrentSize) {
        if (mArrayIndices[current] == variable->id) {
            return true;
        }
        current = mArrayNextIndices[current];
        counter++;
    }
    return false;
}

/// ArrayLinkedVariables.java:465 — public int indexOf(SolverVariable variable)
int ArrayLinkedVariables::indexOf(SolverVariable* variable) {
    if (mHead == NONE) {
        return -1;
    }
    int current = mHead;
    int counter = 0;
    while (current != NONE && counter < mCurrentSize) {
        if (mArrayIndices[current] == variable->id) {
            return current;
        }
        current = mArrayNextIndices[current];
        counter++;
    }
    return -1;
}

/// ArrayLinkedVariables.java:487 — boolean hasAtLeastOnePositiveVariable()
bool ArrayLinkedVariables::hasAtLeastOnePositiveVariable() {
    int current = mHead;
    int counter = 0;
    while (current != NONE && counter < mCurrentSize) {
        if (mArrayValues[current] > 0) {
            return true;
        }
        current = mArrayNextIndices[current];
        counter++;
    }
    return false;
}

/// ArrayLinkedVariables.java:504 — public void invert()
void ArrayLinkedVariables::invert() {
    int current = mHead;
    int counter = 0;
    while (current != NONE && counter < mCurrentSize) {
        mArrayValues[current] *= -1;
        current = mArrayNextIndices[current];
        counter++;
    }
}

/// ArrayLinkedVariables.java:521 — public void divideByAmount(float amount)
void ArrayLinkedVariables::divideByAmount(float amount) {
    int current = mHead;
    int counter = 0;
    while (current != NONE && counter < mCurrentSize) {
        mArrayValues[current] /= amount;
        current = mArrayNextIndices[current];
        counter++;
    }
}

/// ArrayLinkedVariables.java:536 — public int getCurrentSize()
int ArrayLinkedVariables::getCurrentSize() {
    return mCurrentSize;
}

/// ArrayLinkedVariables.java:567 — SolverVariable getPivotCandidate()
SolverVariable* ArrayLinkedVariables::getPivotCandidate() {
    if (mCandidate == nullptr) {
        int current = mHead;
        int counter = 0;
        SolverVariable* pivot = nullptr;
        while (current != NONE && counter < mCurrentSize) {
            if (mArrayValues[current] < 0) {
                SolverVariable* v = mCache->mIndexedVariables[mArrayIndices[current]];
                if (pivot == nullptr || pivot->strength < v->strength) {
                    pivot = v;
                }
            }
            current = mArrayNextIndices[current];
            counter++;
        }
        return pivot;
    }
    return mCandidate;
}

/// ArrayLinkedVariables.java:598 — public SolverVariable getVariable(int index)
SolverVariable* ArrayLinkedVariables::getVariable(int index) {
    int current = mHead;
    int counter = 0;
    while (current != NONE && counter < mCurrentSize) {
        if (counter == index) {
            return mCache->mIndexedVariables[mArrayIndices[current]];
        }
        current = mArrayNextIndices[current];
        counter++;
    }
    return nullptr;
}

/// ArrayLinkedVariables.java:618 — public float getVariableValue(int index)
float ArrayLinkedVariables::getVariableValue(int index) {
    int current = mHead;
    int counter = 0;
    while (current != NONE && counter < mCurrentSize) {
        if (counter == index) {
            return mArrayValues[current];
        }
        current = mArrayNextIndices[current];
        counter++;
    }
    return 0;
}

/// ArrayLinkedVariables.java:638 — public final float get(SolverVariable v)
float ArrayLinkedVariables::get(SolverVariable* v) {
    int current = mHead;
    int counter = 0;
    while (current != NONE && counter < mCurrentSize) {
        if (mArrayIndices[current] == v->id) {
            return mArrayValues[current];
        }
        current = mArrayNextIndices[current];
        counter++;
    }
    return 0;
}

/// ArrayLinkedVariables.java:657 — public int sizeInBytes()
int ArrayLinkedVariables::sizeInBytes() {
    int size = 0;
    size += 3 * (static_cast<int>(mArrayIndices.size()) * 4);
    size += 9 * 4;
    return size;
}

/// ArrayLinkedVariables.java:668 — public void display()
void ArrayLinkedVariables::display() {
    int count = mCurrentSize;
    std::cout << "{ ";
    for (int i = 0; i < count; i++) {
        SolverVariable* v = getVariable(i);
        if (v == nullptr) {
            continue;
        }
        std::cout << v->toString() << " = " << getVariableValue(i) << " ";
    }
    std::cout << " }" << std::endl;
}

/// ArrayLinkedVariables.java:687 — public String toString()
std::string ArrayLinkedVariables::toString() const {
    std::string result;
    int current = mHead;
    int counter = 0;
    while (current != NONE && counter < mCurrentSize) {
        result += " -> ";
        result += std::to_string(mArrayValues[current]) + " : ";
        result += mCache->mIndexedVariables[mArrayIndices[current]]->toString();
        current = mArrayNextIndices[current];
        counter++;
    }
    return result;
}

} // namespace setu::cassowary

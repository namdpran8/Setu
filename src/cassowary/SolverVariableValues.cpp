#include "SolverVariableValues.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <cmath>

namespace setu::cassowary {

// SolverVariableValues.java:25
SolverVariableValues::SolverVariableValues(ArrayRow* row, Cache* cache)
    : mRow(row), mCache(cache) {
    mKeys.resize(mSize, mNone);
    mNextKeys.resize(mSize, mNone);
    mVariables.resize(mSize, mNone);
    mValues.resize(mSize, 0.0f);
    mPrevious.resize(mSize, mNone);
    mNext.resize(mSize, mNone);
    clear();
}

// SolverVariableValues.java:32
int SolverVariableValues::getCurrentSize() {
    return mCount;
}

// SolverVariableValues.java:37
SolverVariable* SolverVariableValues::getVariable(int index) {
    int count = mCount;
    if (count == 0) {
        return nullptr;
    }
    int j = mHead;
    for (int i = 0; i < count; i++) {
        if (i == index && j != mNone) {
            return mCache->mIndexedVariables[mVariables[j]];
        }
        j = mNext[j];
        if (j == mNone) {
            break;
        }
    }
    return nullptr;
}

// SolverVariableValues.java:55
float SolverVariableValues::getVariableValue(int index) {
    int count = mCount;
    int j = mHead;
    for (int i = 0; i < count; i++) {
        if (i == index) {
            return mValues[j];
        }
        j = mNext[j];
        if (j == mNone) {
            break;
        }
    }
    return 0.0f;
}

// SolverVariableValues.java:70
bool SolverVariableValues::contains(SolverVariable* variable) {
    return indexOf(variable) != mNone;
}

// SolverVariableValues.java:75
int SolverVariableValues::indexOf(SolverVariable* variable) {
    if (mCount == 0 || variable == nullptr) {
        return mNone;
    }
    int id = variable->id;
    int key = id % mHashSize;
    int index = mKeys[key];
    if (index == mNone) {
        return mNone;
    }
    if (mVariables[index] == id) {
        return index;
    }
    while (mNextKeys[index] != mNone && mVariables[mNextKeys[index]] != id) {
        index = mNextKeys[index];
    }
    if (mNextKeys[index] == mNone) {
        return mNone;
    }
    if (mVariables[mNextKeys[index]] == id) {
        return mNextKeys[index];
    }
    return mNone;
}

// SolverVariableValues.java:101
float SolverVariableValues::get(SolverVariable* variable) {
    int index = indexOf(variable);
    if (index != mNone) {
        return mValues[index];
    }
    return 0.0f;
}

// SolverVariableValues.java:109
void SolverVariableValues::clear() {
    int count = mCount;
    int j = mHead;
    for (int i = 0; i < count; i++) {
        SolverVariable* variable = mCache->mIndexedVariables[mVariables[j]];
        if (variable != nullptr) {
            variable->removeFromRow(mRow);
        }
        j = mNext[j];
        if (j == mNone) {
            break;
        }
    }
    for (int i = 0; i < mSize; i++) {
        mVariables[i] = mNone;
        mNextKeys[i] = mNone;
    }
    for (int i = 0; i < mHashSize; i++) {
        mKeys[i] = mNone;
    }
    mCount = 0;
    mHead = mNone;
}

// SolverVariableValues.java:132
void SolverVariableValues::increaseSize() {
    int size = mSize * 2;
    mVariables.resize(size, mNone);
    mValues.resize(size, 0.0f);
    mPrevious.resize(size, mNone);
    mNext.resize(size, mNone);
    mNextKeys.resize(size, mNone);
    for (int i = mSize; i < size; i++) {
        mVariables[i] = mNone;
        mNextKeys[i] = mNone;
    }
    mSize = size;
    mHashSize = mSize;
    mKeys.assign(mHashSize, mNone);

    for (int i = 0, j = mHead; i < mCount; i++) {
        int index = j;
        int id = mVariables[index];
        int key = id % mHashSize;
        int keyIndex = mKeys[key];
        if (keyIndex == mNone) {
            mKeys[key] = index;
        } else {
            while (mNextKeys[keyIndex] != mNone) {
                keyIndex = mNextKeys[keyIndex];
            }
            mNextKeys[keyIndex] = index;
        }
        mNextKeys[index] = mNone;
        j = mNext[j];
        if (j == mNone) {
            break;
        }
    }
}

// SolverVariableValues.java:169
void SolverVariableValues::addToHashMap(SolverVariable* variable, int index) {
    int key = variable->id % mHashSize;
    int keyIndex = mKeys[key];
    if (keyIndex == mNone) {
        mKeys[key] = index;
    } else {
        while (mNextKeys[keyIndex] != mNone) {
            keyIndex = mNextKeys[keyIndex];
        }
        mNextKeys[keyIndex] = index;
    }
    mNextKeys[index] = mNone;
}

// SolverVariableValues.java:185
void SolverVariableValues::removeFromHashMap(SolverVariable* variable) {
    int key = variable->id % mHashSize;
    int keyIndex = mKeys[key];
    if (keyIndex == mNone) {
        return;
    }
    int id = variable->id;
    if (mVariables[keyIndex] == id) {
        mKeys[key] = mNextKeys[keyIndex];
        mNextKeys[keyIndex] = mNone;
    } else {
        while (mNextKeys[keyIndex] != mNone && mVariables[mNextKeys[keyIndex]] != id) {
            keyIndex = mNextKeys[keyIndex];
        }
        int currentKeyIndex = mNextKeys[keyIndex];
        if (currentKeyIndex != mNone && mVariables[currentKeyIndex] == id) {
            mNextKeys[keyIndex] = mNextKeys[currentKeyIndex];
            mNextKeys[currentKeyIndex] = mNone;
        }
    }
}

// SolverVariableValues.java:214
void SolverVariableValues::addVariable(int index, SolverVariable* variable, float value) {
    mVariables[index] = variable->id;
    mValues[index] = value;
    mPrevious[index] = mNone;
    mNext[index] = mNone;
    variable->addToRow(mRow);
    variable->usageInRowCount++;
    mCount++;
}

// SolverVariableValues.java:224
int SolverVariableValues::findEmptySlot() {
    for (int i = 0; i < mSize; i++) {
        if (mVariables[i] == mNone) {
            return i;
        }
    }
    return -1;
}

// SolverVariableValues.java:233
void SolverVariableValues::insertVariable(int index, SolverVariable* variable, float value) {
    int availableSlot = findEmptySlot();
    addVariable(availableSlot, variable, value);
    if (index != mNone) {
        mPrevious[availableSlot] = index;
        mNext[availableSlot] = mNext[index];
        mNext[index] = availableSlot;
    } else {
        mPrevious[availableSlot] = mNone;
        if (mCount > 0) {
            mNext[availableSlot] = mHead;
            mHead = availableSlot;
        } else {
            mNext[availableSlot] = mNone;
        }
    }
    if (mNext[availableSlot] != mNone) {
        mPrevious[mNext[availableSlot]] = availableSlot;
    }
    addToHashMap(variable, availableSlot);
}

// SolverVariableValues.java:256
void SolverVariableValues::put(SolverVariable* variable, float value) {
    if (value > -sEpsilon && value < sEpsilon) {
        remove(variable, true);
        return;
    }
    if (mCount == 0) {
        addVariable(0, variable, value);
        addToHashMap(variable, 0);
        mHead = 0;
    } else {
        int index = indexOf(variable);
        if (index != mNone) {
            mValues[index] = value;
        } else {
            if (mCount + 1 >= mSize) {
                increaseSize();
            }
            int count = mCount;
            int previousItem = -1;
            int j = mHead;
            for (int i = 0; i < count; i++) {
                if (mVariables[j] == variable->id) {
                    mValues[j] = value;
                    return;
                }
                if (mVariables[j] < variable->id) {
                    previousItem = j;
                }
                j = mNext[j];
                if (j == mNone) {
                    break;
                }
            }
            insertVariable(previousItem, variable, value);
        }
    }
}

// SolverVariableValues.java:311
int SolverVariableValues::sizeInBytes() {
    return 0;
}

// SolverVariableValues.java:316
float SolverVariableValues::remove(SolverVariable* v, bool removeFromDefinition) {
    int index = indexOf(v);
    if (index == mNone) {
        return 0.0f;
    }
    removeFromHashMap(v);
    float value = mValues[index];
    if (mHead == index) {
        mHead = mNext[index];
    }
    mVariables[index] = mNone;
    if (mPrevious[index] != mNone) {
        mNext[mPrevious[index]] = mNext[index];
    }
    if (mNext[index] != mNone) {
        mPrevious[mNext[index]] = mPrevious[index];
    }
    mCount--;
    if (removeFromDefinition) {
        v->removeFromRow(mRow);
    }
    return value;
}

// SolverVariableValues.java:353
void SolverVariableValues::add(SolverVariable* v, float value, bool removeFromDefinition) {
    if (value > -sEpsilon && value < sEpsilon) {
        return;
    }
    int index = indexOf(v);
    if (index == mNone) {
        put(v, value);
    } else {
        mValues[index] += value;
        if (mValues[index] > -sEpsilon && mValues[index] < sEpsilon) {
            mValues[index] = 0.0f;
            remove(v, removeFromDefinition);
        }
    }
}

// SolverVariableValues.java:374
float SolverVariableValues::use(ArrayRow* definition, bool removeFromDefinition) {
    float value = get(definition->mVariable);
    remove(definition->mVariable, removeFromDefinition);
    
    // ACTIVE PATH: Port of the index scan logic from Java's "else" branch
    SolverVariableValues* defVars = dynamic_cast<SolverVariableValues*>(definition->variables.get());
    if (defVars) {
        int defSize = defVars->getCurrentSize();
        int j = 0;
        for (int i = 0; j < defSize; i++) {
            if (defVars->mVariables[i] != mNone) {
                float val = defVars->mValues[i];
                SolverVariable* v = mCache->mIndexedVariables[defVars->mVariables[i]];
                add(v, val * value, removeFromDefinition);
                j++;
            }
        }
    } else {
        // Fallback for completeness, though optimization assumes SolverVariableValues
        int defSize = definition->variables->getCurrentSize();
        for (int i = 0; i < defSize; i++) {
            SolverVariable* v = definition->variables->getVariable(i);
            float val = definition->variables->getVariableValue(i);
            add(v, val * value, removeFromDefinition);
        }
    }
    return value;
}

// SolverVariableValues.java:449
void SolverVariableValues::invert() {
    int count = mCount;
    int j = mHead;
    for (int i = 0; i < count; i++) {
        mValues[j] = mValues[j] * -1;
        j = mNext[j];
        if (j == mNone) {
            break;
        }
    }
}

// SolverVariableValues.java:464
void SolverVariableValues::divideByAmount(float amount) {
    int count = mCount;
    int j = mHead;
    for (int i = 0; i < count; i++) {
        mValues[j] /= amount;
        j = mNext[j];
        if (j == mNone) {
            break;
        }
    }
}

// SolverVariableValues.java:482
std::string SolverVariableValues::toString() {
    std::stringstream ss;
    ss << reinterpret_cast<uintptr_t>(this) << " [ ";
    int j = mHead;
    for (int i = 0; i < mCount; i++) {
        ss << mVariables[j] << " = " << mValues[j] << " ";
        j = mNext[j];
        if (j == mNone) {
            break;
        }
    }
    ss << "]";
    return ss.str();
}

// SolverVariableValues.java:493
void SolverVariableValues::display() {
    int count = mCount;
    std::cout << "{ ";
    int j = mHead;
    for (int i = 0; i < count; i++) {
        SolverVariable* v = getVariable(i);
        if (v == nullptr) {
            continue;
        }
        std::cout << v->toString() << " = " << getVariableValue(i) << " ";
        j = mNext[j];
        if (j == mNone) {
            break;
        }
    }
    std::cout << " }" << std::endl;
}

} // namespace setu::cassowary

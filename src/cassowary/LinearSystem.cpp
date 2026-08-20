#include "LinearSystem.h"

#include <cmath>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <limits>

#include "PriorityGoalRow.h"
#include "GoalRow.h"
#include "SolverVariableValues.h"
#include "ArrayLinkedVariables.h"
// For ConstraintAnchor and ConstraintWidget
#include "ConstraintAnchor.h"
#include "ConstraintWidget.h"

namespace setu::cassowary {

LinearSystem::ValuesRow::ValuesRow(Cache* cache) : ArrayRow(cache) {
    // In Java: variables = new SolverVariableValues(this, cache);
    // Replace the ArrayLinkedVariables created by ArrayRow(cache) with SolverVariableValues.
    variables = std::make_unique<SolverVariableValues>(this, cache);
}

LinearSystem::LinearSystem() {
    mRows.resize(mTableSize, nullptr);
    mAlreadyTestedCandidates.assign(mTableSize, false);
    releaseRows();
    mGoal = std::make_unique<PriorityGoalRow>(&mCache);
    if (OPTIMIZED_ENGINE) {
        mTempGoal = std::make_unique<ValuesRow>(&mCache);
    } else {
        mTempGoal = std::make_unique<ArrayRow>(&mCache);
    }
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:107
void LinearSystem::fillMetrics(Metrics* metrics) {
    sMetrics = metrics;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:111
Metrics* LinearSystem::getMetrics() {
    return sMetrics;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:144
void LinearSystem::increaseTableSize() {
    if constexpr (DEBUG) {
        std::cout << "###########################\n";
        std::cout << "### INCREASE TABLE TO " << (mTableSize * 2) << " (num rows: "
                  << mNumRows << ", num cols: " << mNumColumns << "/" << mMaxColumns << ")\n";
        std::cout << "###########################\n";
    }
    mTableSize *= 2;
    mRows.resize(mTableSize, nullptr);
    mCache.mIndexedVariables.resize(mTableSize, nullptr);
    mAlreadyTestedCandidates.assign(mTableSize, false);
    mMaxColumns = mTableSize;
    mMaxRows = mTableSize;
    if (sMetrics != nullptr) {
        sMetrics->tableSizeIncrease++;
        sMetrics->maxTableSize = std::max(sMetrics->maxTableSize, (long long)mTableSize);
        sMetrics->lastTableSize = sMetrics->maxTableSize;
    }
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:167
void LinearSystem::releaseRows() {
    if (OPTIMIZED_ENGINE) {
        for (int i = 0; i < mNumRows; i++) {
            ArrayRow* row = mRows[i];
            if (row != nullptr) {
                mCache.mOptimizedArrayRowPool.release(row);
            }
            mRows[i] = nullptr;
        }
    } else {
        for (int i = 0; i < mNumRows; i++) {
            ArrayRow* row = mRows[i];
            if (row != nullptr) {
                mCache.mArrayRowPool.release(row);
            }
            mRows[i] = nullptr;
        }
    }
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:190
void LinearSystem::reset() {
    if constexpr (DEBUG) {
        std::cout << "##################\n";
        std::cout << "## RESET SYSTEM ##\n";
        std::cout << "##################\n";
    }
    for (size_t i = 0; i < mCache.mIndexedVariables.size(); i++) {
        SolverVariable* variable = mCache.mIndexedVariables[i];
        if (variable != nullptr) {
            variable->reset();
        }
    }
    mCache.mSolverVariablePool.releaseAll(mPoolVariables, mPoolVariablesCount);
    mPoolVariablesCount = 0;

    std::fill(mCache.mIndexedVariables.begin(), mCache.mIndexedVariables.end(), nullptr);
    mVariables.clear();
    mVariablesID = 0;
    mGoal->clear();
    mNumColumns = 1;
    for (int i = 0; i < mNumRows; i++) {
        if (mRows[i] != nullptr) {
            mRows[i]->mUsed = false;
        }
    }
    releaseRows();
    mNumRows = 0;
    if (OPTIMIZED_ENGINE) {
        mTempGoal = std::make_unique<ValuesRow>(&mCache);
    } else {
        mTempGoal = std::make_unique<ArrayRow>(&mCache);
    }
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:231
SolverVariable* LinearSystem::createObjectVariable(ConstraintAnchor* anchor) {
    if (anchor == nullptr) {
        return nullptr;
    }
    if (mNumColumns + 1 >= mMaxColumns) {
        increaseTableSize();
    }
    SolverVariable* variable = anchor->getSolverVariable();
    if (variable == nullptr) {
        anchor->resetSolverVariable(&mCache);
        variable = anchor->getSolverVariable();
    }
    if (variable->id == -1 || variable->id > mVariablesID || mCache.mIndexedVariables[variable->id] == nullptr) {
        if (variable->id != -1) {
            variable->reset();
        }
        mVariablesID++;
        mNumColumns++;
        variable->id = mVariablesID;
        variable->mType = SolverVariable::Type::UNRESTRICTED;
        mCache.mIndexedVariables[mVariablesID] = variable;
    }
    return variable;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:265
ArrayRow* LinearSystem::createRow() {
    ArrayRow* row;
    if (OPTIMIZED_ENGINE) {
        row = mCache.mOptimizedArrayRowPool.acquire();
        if (row == nullptr) {
            row = new ValuesRow(&mCache);
            OPTIMIZED_ARRAY_ROW_CREATION++;
        } else {
            row->reset();
        }
    } else {
        row = mCache.mArrayRowPool.acquire();
        if (row == nullptr) {
            row = new ArrayRow(&mCache);
            ARRAY_ROW_CREATION++;
        } else {
            row->reset();
        }
    }
    SolverVariable::increaseErrorId();
    return row;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:289
SolverVariable* LinearSystem::createSlackVariable() {
    if (sMetrics != nullptr) {
        sMetrics->slackvariables++;
    }
    if (mNumColumns + 1 >= mMaxColumns) {
        increaseTableSize();
    }
    SolverVariable* variable = acquireSolverVariable(SolverVariable::Type::SLACK, "");
    mVariablesID++;
    mNumColumns++;
    variable->id = mVariablesID;
    mCache.mIndexedVariables[mVariablesID] = variable;
    return variable;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:305
SolverVariable* LinearSystem::createExtraVariable() {
    if (sMetrics != nullptr) {
        sMetrics->extravariables++;
    }
    if (mNumColumns + 1 >= mMaxColumns) {
        increaseTableSize();
    }
    SolverVariable* variable = acquireSolverVariable(SolverVariable::Type::SLACK, "");
    mVariablesID++;
    mNumColumns++;
    variable->id = mVariablesID;
    mCache.mIndexedVariables[mVariablesID] = variable;
    return variable;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:328
void LinearSystem::addSingleError(ArrayRow* row, int sign, int strength) {
    std::string prefix = "";
    if constexpr (DEBUG) {
        if (sign > 0) {
            prefix = "ep";
        } else {
            prefix = "em";
        }
        prefix = "em";
    }
    SolverVariable* error = createErrorVariable(strength, prefix);
    row->addSingleError(error, sign);
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:342
SolverVariable* LinearSystem::createVariable(const std::string& name, SolverVariable::Type type) {
    if (sMetrics != nullptr) {
        sMetrics->variables++;
    }
    if (mNumColumns + 1 >= mMaxColumns) {
        increaseTableSize();
    }
    SolverVariable* variable = acquireSolverVariable(type, "");
    variable->setName(name);
    mVariablesID++;
    mNumColumns++;
    variable->id = mVariablesID;
    mVariables[name] = variable;
    mCache.mIndexedVariables[mVariablesID] = variable;
    return variable;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:363
SolverVariable* LinearSystem::createErrorVariable(int strength, const std::string& prefix) {
    if (sMetrics != nullptr) {
        sMetrics->errors++;
    }
    if (mNumColumns + 1 >= mMaxColumns) {
        increaseTableSize();
    }
    SolverVariable* variable = acquireSolverVariable(SolverVariable::Type::ERROR, prefix);
    mVariablesID++;
    mNumColumns++;
    variable->id = mVariablesID;
    variable->strength = strength;
    mCache.mIndexedVariables[mVariablesID] = variable;
    mGoal->addError(variable);
    return variable;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:386
SolverVariable* LinearSystem::acquireSolverVariable(SolverVariable::Type type, const std::string& prefix) {
    SolverVariable* variable = mCache.mSolverVariablePool.acquire();
    if (variable == nullptr) {
        variable = new SolverVariable(type, prefix);
        variable->setType(type, prefix);
    } else {
        variable->reset();
        variable->setType(type, prefix);
    }
    if (mPoolVariablesCount >= sPoolSize) {
        // Pool size handled by vector capacity dynamically.
    }
    mPoolVariables.push_back(variable);
    mPoolVariablesCount++;
    return variable;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:412
Row* LinearSystem::getGoal() {
    return mGoal.get();
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:416
ArrayRow* LinearSystem::getRow(int n) {
    return mRows[n];
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:420
float LinearSystem::getValueFor(const std::string& name) {
    SolverVariable* v = getVariable(name, SolverVariable::Type::UNRESTRICTED);
    if (v == nullptr) {
        return 0;
    }
    return v->computedValue;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:429
int LinearSystem::getObjectVariableValue(ConstraintAnchor* anchor) {
    // TODO: Stub for Chain.USE_CHAIN_OPTIMIZATION
    /*
    if (Chain::USE_CHAIN_OPTIMIZATION) {
        if (anchor->hasFinalValue()) {
            return anchor->getFinalValue();
        }
    }
    */
    SolverVariable* variable = anchor->getSolverVariable();
    if (variable != nullptr) {
        return (int) (variable->computedValue + 0.5f);
    }
    return 0;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:450
SolverVariable* LinearSystem::getVariable(const std::string& name, SolverVariable::Type type) {
    auto it = mVariables.find(name);
    if (it == mVariables.end()) {
        return createVariable(name, type);
    }
    return it->second;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:468
void LinearSystem::minimize() {
    if (sMetrics != nullptr) {
        sMetrics->minimize++;
    }
    if (mGoal->isEmpty()) {
        if constexpr (DEBUG) {
            std::cout << "\n*** SKIPPING MINIMIZE! ***\n\n";
        }
        computeValues();
        return;
    }
    if constexpr (DEBUG) {
        std::cout << "\n*** MINIMIZE ***\n\n";
    }
    if (graphOptimizer || newgraphOptimizer) {
        if (sMetrics != nullptr) {
            sMetrics->graphOptimizer++;
        }
        bool fullySolved = true;
        for (int i = 0; i < mNumRows; i++) {
            ArrayRow* r = mRows[i];
            if (!r->mIsSimpleDefinition) {
                fullySolved = false;
                break;
            }
        }
        if (!fullySolved) {
            minimizeGoal(mGoal.get());
        } else {
            if (sMetrics != nullptr) {
                sMetrics->fullySolved++;
            }
            computeValues();
        }
    } else {
        minimizeGoal(mGoal.get());
    }
    if constexpr (DEBUG) {
        std::cout << "\n*** END MINIMIZE ***\n\n";
    }
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:515
void LinearSystem::minimizeGoal(Row* goal) {
    if (sMetrics != nullptr) {
        sMetrics->minimizeGoal++;
        sMetrics->maxVariables = std::max(sMetrics->maxVariables, (long long)mNumColumns);
        sMetrics->maxRows = std::max(sMetrics->maxRows, (long long)mNumRows);
    }
    if constexpr (DEBUG) {
        std::cout << "minimize goal: " << goal << "\n";
    }
    if constexpr (DEBUG) {
        displayReadableRows();
    }
    enforceBFS(goal);
    if constexpr (DEBUG) {
        std::cout << "Goal after enforcing BFS " << goal << "\n";
        displayReadableRows();
    }
    optimize(goal, false);
    if constexpr (DEBUG) {
        std::cout << "Goal after optimization " << goal << "\n";
        displayReadableRows();
    }
    computeValues();
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:544
void LinearSystem::cleanupRows() {
    int i = 0;
    while (i < mNumRows) {
        ArrayRow* current = mRows[i];
        if (current->variables->getCurrentSize() == 0) {
            current->mIsSimpleDefinition = true;
        }
        if (current->mIsSimpleDefinition) {
            current->mVariable->computedValue = current->mConstantValue;
            current->mVariable->removeFromRow(current);
            for (int j = i; j < mNumRows - 1; j++) {
                mRows[j] = mRows[j + 1];
            }
            mRows[mNumRows - 1] = nullptr;
            mNumRows--;
            i--;
            if (OPTIMIZED_ENGINE) {
                mCache.mOptimizedArrayRowPool.release(current);
            } else {
                mCache.mArrayRowPool.release(current);
            }
        }
        i++;
    }
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:575
void LinearSystem::addConstraint(ArrayRow* row) {
    if (row == nullptr) {
        return;
    }
    if (sMetrics != nullptr) {
        sMetrics->constraints++;
        if (row->mIsSimpleDefinition) {
            sMetrics->simpleconstraints++;
        }
    }
    if (mNumRows + 1 >= mMaxRows || mNumColumns + 1 >= mMaxColumns) {
        increaseTableSize();
    }
    if constexpr (DEBUG) {
        std::cout << "addConstraint <" << row->toReadableString() << ">\n";
        displayReadableRows();
    }

    bool added = false;
    if (!row->mIsSimpleDefinition) {
        row->updateFromSystem(this);

        if (row->isEmpty()) {
            std::cout << "DEBUG: Exiting because isEmpty\n";
            return;
        }

        row->ensurePositiveConstant();

        if constexpr (DEBUG) {
            std::cout << "addConstraint, updated row : " << row->toReadableString() << "\n";
        }

        if (row->chooseSubject(this)) {
            SolverVariable* extra = createExtraVariable();
            row->mVariable = extra;
            int numRows = mNumRows;
            addRow(row);
            if (mNumRows == numRows + 1) {
                added = true;
                mTempGoal->initFromRow(row);
                optimize(mTempGoal.get(), true);
                if (extra->mDefinitionId == -1) {
                    if constexpr (DEBUG) {
                        std::cout << "row added is 0, so get rid of it\n";
                    }
                    if (row->mVariable == extra) {
                        SolverVariable* pivotCandidate = row->pickPivot(extra);
                        if (pivotCandidate != nullptr) {
                            if (sMetrics != nullptr) {
                                sMetrics->pivots++;
                            }
                            row->pivot(pivotCandidate);
                        }
                    }
                    if (!row->mIsSimpleDefinition) {
                        row->mVariable->updateReferencesWithNewDefinition(this, row);
                    }
                    if (OPTIMIZED_ENGINE) {
                        mCache.mOptimizedArrayRowPool.release(row);
                    } else {
                        mCache.mArrayRowPool.release(row);
                    }
                    mNumRows--;
                }
            }
        }

        if (!row->hasKeyVariable()) {
            if constexpr (DEBUG) {
                std::cout << "No variable found to pivot on " << row->toReadableString() << "\n";
                displayReadableRows();
            }
            std::cout << "DEBUG: Exiting because !hasKeyVariable\n";
            return;
        }
    }
    if (!added) {
        std::cout << "DEBUG: Reached addRow\n";
        addRow(row);
    }
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:661
void LinearSystem::addRow(ArrayRow* row) {
    if (SIMPLIFY_SYNONYMS && row->mIsSimpleDefinition) {
        std::cout << "Simple definition for " << row->mVariable->getName() << " = " << row->mConstantValue << "\n";
        row->mVariable->setFinalValue(this, row->mConstantValue);
    } else {
        mRows[mNumRows] = row;
        row->mVariable->mDefinitionId = mNumRows;
        mNumRows++;
        row->mVariable->updateReferencesWithNewDefinition(this, row);
    }
    if constexpr (DEBUG) {
        // std::cout << "Row added: " << row << "\n";
        std::cout << "here is the system:\n";
        displayReadableRows();
    }
    if (SIMPLIFY_SYNONYMS && hasSimpleDefinition) {
        for (int i = 0; i < mNumRows; i++) {
            if (mRows[i] == nullptr) {
                std::cout << "WTF\n";
            }
            if (mRows[i] != nullptr && mRows[i]->mIsSimpleDefinition) {
                ArrayRow* removedRow = mRows[i];
                removedRow->mVariable->setFinalValue(this, removedRow->mConstantValue);
                if (OPTIMIZED_ENGINE) {
                    mCache.mOptimizedArrayRowPool.release(removedRow);
                } else {
                    mCache.mArrayRowPool.release(removedRow);
                }
                mRows[i] = nullptr;
                int lastRow = i + 1;
                for (int j = i + 1; j < mNumRows; j++) {
                    mRows[j - 1] = mRows[j];
                    if (mRows[j - 1]->mVariable->mDefinitionId == j) {
                        mRows[j - 1]->mVariable->mDefinitionId = j - 1;
                    }
                    lastRow = j;
                }
                if (lastRow < mNumRows) {
                    mRows[lastRow] = nullptr;
                }
                mNumRows--;
                i--;
            }
        }
        hasSimpleDefinition = false;
    }
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:709
void LinearSystem::removeRow(ArrayRow* row) {
    if (row->mIsSimpleDefinition && row->mVariable != nullptr) {
        if (row->mVariable->mDefinitionId != -1) {
            for (int i = row->mVariable->mDefinitionId; i < mNumRows - 1; i++) {
                SolverVariable* rowVariable = mRows[i + 1]->mVariable;
                if (rowVariable->mDefinitionId == i + 1) {
                    rowVariable->mDefinitionId = i;
                }
                mRows[i] = mRows[i + 1];
            }
            mNumRows--;
        }
        if (!row->mVariable->isFinalValue) {
            row->mVariable->setFinalValue(this, row->mConstantValue);
        }
        if (OPTIMIZED_ENGINE) {
            mCache.mOptimizedArrayRowPool.release(row);
        } else {
            mCache.mArrayRowPool.release(row);
        }
    }
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:739
int LinearSystem::optimize(Row* goal, bool b) {
    if (sMetrics != nullptr) {
        sMetrics->optimize++;
    }
    bool done = false;
    int tries = 0;
    for (int i = 0; i < mNumColumns; i++) {
        mAlreadyTestedCandidates[i] = false;
    }

    if constexpr (DEBUG) {
        std::cout << "\n****************************\n";
        std::cout << "*       OPTIMIZATION       *\n";
        std::cout << "* mNumColumns: " << mNumColumns << "\n";
        // std::cout << "* GOAL: " << goal << " " << b << "\n";
        std::cout << "****************************\n\n";
    }

    while (!done) {
        if (sMetrics != nullptr) {
            sMetrics->iterations++;
        }
        tries++;
        if constexpr (DEBUG) {
            std::cout << "\n******************************\n";
            std::cout << "* iteration: " << tries << std::endl;
        }
        if (tries >= 2 * mNumColumns) {
            if constexpr (DEBUG) {
                std::cout << "=> Exit optimization because tries " << tries << " >= " << (2 * mNumColumns) << std::endl;
            }
            return tries;
        }

        if (goal->getKey() != nullptr) {
            mAlreadyTestedCandidates[goal->getKey()->id] = true;
        }
        SolverVariable* pivotCandidate = goal->getPivotCandidate(this, mAlreadyTestedCandidates);
        if constexpr (DEBUG) {
            // std::cout << "* Pivot candidate: " << pivotCandidate << "\n";
            std::cout << "******************************\n\n";
        }
        if (pivotCandidate != nullptr) {
            if (mAlreadyTestedCandidates[pivotCandidate->id]) {
                if constexpr (DEBUG) {
                    std::cout << "* Pivot candidate already tested, let's bail\n";
                }
                return tries;
            } else {
                mAlreadyTestedCandidates[pivotCandidate->id] = true;
            }
        }

        if (pivotCandidate != nullptr) {
            if constexpr (DEBUG) {
                std::cout << "valid pivot candidate\n";
            }
            float min = std::numeric_limits<float>::max();
            int pivotRowIndex = -1;

            for (int i = 0; i < mNumRows; i++) {
                ArrayRow* current = mRows[i];
                SolverVariable* variable = current->mVariable;
                if (variable->mType == SolverVariable::Type::UNRESTRICTED) {
                    continue;
                }
                if (current->mIsSimpleDefinition) {
                    continue;
                }

                if (current->hasVariable(pivotCandidate)) {
                    if constexpr (DEBUG) {
                        std::cout << "equation " << i << " contains pivotCandidate\n";
                    }
                    float a_j = current->variables->get(pivotCandidate);
                    if (a_j < 0) {
                        float value = -current->mConstantValue / a_j;
                        if (value < min) {
                            min = value;
                            pivotRowIndex = i;
                        }
                    }
                }
            }

            if (pivotRowIndex > -1) {
                if constexpr (DEBUG) {
                    std::cout << "We pivot on " << pivotRowIndex << "\n";
                }
                ArrayRow* pivotEquation = mRows[pivotRowIndex];
                pivotEquation->mVariable->mDefinitionId = -1;
                if (sMetrics != nullptr) {
                    sMetrics->pivots++;
                }
                pivotEquation->pivot(pivotCandidate);
                pivotEquation->mVariable->mDefinitionId = pivotRowIndex;
                pivotEquation->mVariable->updateReferencesWithNewDefinition(this, pivotEquation);
                if constexpr (DEBUG) {
                    std::cout << "new system after pivot:\n";
                    displayReadableRows();
                }
            } else {
                if constexpr (DEBUG) {
                    std::cout << "we couldn't find an equation to pivot upon\n";
                }
            }
        } else {
            if constexpr (DEBUG) {
                std::cout << "no more candidate goals to pivot on, let's exit\n";
            }
            done = true;
        }
    }
    return tries;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:893
int LinearSystem::enforceBFS(Row* goal) {
    int tries = 0;
    bool done;

    if constexpr (DEBUG) {
        std::cout << "\n#################\n";
        std::cout << "# ENFORCING BFS #\n";
        std::cout << "#################\n\n";
    }

    bool infeasibleSystem = false;
    for (int i = 0; i < mNumRows; i++) {
        SolverVariable* variable = mRows[i]->mVariable;
        if (variable->mType == SolverVariable::Type::UNRESTRICTED) {
            continue;
        }
        if (mRows[i]->mConstantValue < 0) {
            infeasibleSystem = true;
            break;
        }
    }

    if (infeasibleSystem) {
        if constexpr (DEBUG) {
            std::cout << "the current system is infeasible, let's try to fix this.\n";
        }

        done = false;
        tries = 0;
        while (!done) {
            if (sMetrics != nullptr) {
                sMetrics->bfs++;
            }
            tries++;
            if constexpr (DEBUG) {
                std::cout << "iteration on infeasible system " << tries << std::endl;
            }
            float min = std::numeric_limits<float>::max();
            int strength = 0;
            int pivotRowIndex = -1;
            int pivotColumnIndex = -1;

            for (int i = 0; i < mNumRows; i++) {
                ArrayRow* current = mRows[i];
                SolverVariable* variable = current->mVariable;
                if (variable->mType == SolverVariable::Type::UNRESTRICTED) {
                    continue;
                }
                if (current->mIsSimpleDefinition) {
                    continue;
                }
                if (current->mConstantValue < 0) {
                    if constexpr (DEBUG) {
                        std::cout << "looking at pivoting on row\n";
                    }
                    if (SKIP_COLUMNS) {
                        const int size = current->variables->getCurrentSize();
                        for (int j = 0; j < size; j++) {
                            SolverVariable* candidate = current->variables->getVariable(j);
                            float a_j = current->variables->get(candidate);
                            if (a_j <= 0) {
                                continue;
                            }
                            if constexpr (DEBUG) {
                                std::cout << "candidate for pivot\n";
                            }
                            for (int k = 0; k < SolverVariable::MAX_STRENGTH; k++) {
                                float value = candidate->mStrengthVector[k] / a_j;
                                if ((value < min && k == strength) || k > strength) {
                                    min = value;
                                    pivotRowIndex = i;
                                    pivotColumnIndex = candidate->id;
                                    strength = k;
                                }
                            }
                        }
                    } else {
                        for (int j = 1; j < mNumColumns; j++) {
                            SolverVariable* candidate = mCache.mIndexedVariables[j];
                            float a_j = current->variables->get(candidate);
                            if (a_j <= 0) {
                                continue;
                            }
                            if constexpr (DEBUG) {
                                std::cout << "candidate for pivot\n";
                            }
                            for (int k = 0; k < SolverVariable::MAX_STRENGTH; k++) {
                                float value = candidate->mStrengthVector[k] / a_j;
                                if ((value < min && k == strength) || k > strength) {
                                    min = value;
                                    pivotRowIndex = i;
                                    pivotColumnIndex = j;
                                    strength = k;
                                }
                            }
                        }
                    }
                }
            }

            if (pivotRowIndex != -1) {
                ArrayRow* pivotEquation = mRows[pivotRowIndex];
                if constexpr (DEBUG) {
                    std::cout << "Pivoting on row\n";
                }
                pivotEquation->mVariable->mDefinitionId = -1;
                if (sMetrics != nullptr) {
                    sMetrics->pivots++;
                }
                pivotEquation->pivot(mCache.mIndexedVariables[pivotColumnIndex]);
                pivotEquation->mVariable->mDefinitionId = pivotRowIndex;
                pivotEquation->mVariable->updateReferencesWithNewDefinition(this, pivotEquation);

                if constexpr (DEBUG) {
                    std::cout << "new goal after pivot:\n";
                    displayRows();
                }
            } else {
                done = true;
            }
            if (tries > mNumColumns / 2) {
                done = true;
            }
        }
    }

    if constexpr (DEBUG) {
        std::cout << "the current system should now be feasible [" << infeasibleSystem << "] after " << tries << " iterations\n";
        displayReadableRows();

        infeasibleSystem = false;
        for (int i = 0; i < mNumRows; i++) {
            SolverVariable* variable = mRows[i]->mVariable;
            if (variable->mType == SolverVariable::Type::UNRESTRICTED) {
                continue;
            }
            if (mRows[i]->mConstantValue < 0) {
                infeasibleSystem = true;
                break;
            }
        }

        if (DEBUG && infeasibleSystem) {
            std::cout << "IMPOSSIBLE SYSTEM, WTF\n";
            assert(!infeasibleSystem && "IMPOSSIBLE SYSTEM");
        }
        if (infeasibleSystem) {
            return tries;
        }
    }

    return tries;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1064
void LinearSystem::computeValues() {
    for (int i = 0; i < mNumRows; i++) {
        ArrayRow* row = mRows[i];
        row->mVariable->computedValue = row->mConstantValue;
    }
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1075
void LinearSystem::displayRows() {
    displaySolverVariables();
    std::string s = "";
    // skipping actual conversion for brevity of debug display in c++ port
    std::cout << s;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1088
void LinearSystem::displayReadableRows() {
    displaySolverVariables();
    std::string s = " num vars " + std::to_string(mVariablesID) + "\n";
    // skipping actual debug print code ...
    std::cout << s;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1119
void LinearSystem::displayVariablesReadableRows() {
    displaySolverVariables();
    std::string s = "";
    std::cout << s;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1135
int LinearSystem::getMemoryUsed() {
    int actualRowSize = 0;
    for (int i = 0; i < mNumRows; i++) {
        if (mRows[i] != nullptr) {
            actualRowSize += mRows[i]->sizeInBytes();
        }
    }
    return actualRowSize;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1146
int LinearSystem::getNumEquations() {
    return mNumRows;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1151
int LinearSystem::getNumVariables() {
    return mVariablesID;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1158
void LinearSystem::displaySystemInformation() {
    // ...
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1183
void LinearSystem::displaySolverVariables() {
    std::string s = "Display Rows (" + std::to_string(mNumRows) + "x" + std::to_string(mNumColumns) + ")\n";
    std::cout << s;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1197
std::string LinearSystem::getDisplaySize(int n) {
    int mb = (n * 4) / 1024 / 1024;
    if (mb > 0) {
        return std::to_string(mb) + " Mb";
    }
    int kb = (n * 4) / 1024;
    if (kb > 0) {
        return std::to_string(kb) + " Kb";
    }
    return std::to_string(n * 4) + " bytes";
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1209
Cache* LinearSystem::getCache() {
    return &mCache;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1213
std::string LinearSystem::getDisplayStrength(int strength) {
    if (strength == SolverVariable::STRENGTH_LOW) return "LOW";
    if (strength == SolverVariable::STRENGTH_MEDIUM) return "MEDIUM";
    if (strength == SolverVariable::STRENGTH_HIGH) return "HIGH";
    if (strength == SolverVariable::STRENGTH_HIGHEST) return "HIGHEST";
    if (strength == SolverVariable::STRENGTH_EQUALITY) return "EQUALITY";
    if (strength == SolverVariable::STRENGTH_FIXED) return "FIXED";
    if (strength == SolverVariable::STRENGTH_BARRIER) return "BARRIER";
    return "NONE";
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1250
void LinearSystem::addGreaterThan(SolverVariable* a, SolverVariable* b, int margin, int strength) {
    if constexpr (DEBUG_CONSTRAINTS) {
        std::cout << "-> addGreaterThan\n";
    }
    ArrayRow* row = createRow();
    SolverVariable* slack = createSlackVariable();
    slack->strength = 0;
    row->createRowGreaterThan(a, b, slack, margin);
    if (strength != SolverVariable::STRENGTH_FIXED) {
        float slackValue = row->variables->get(slack);
        addSingleError(row, (int) (-1 * slackValue), strength);
    }
    addConstraint(row);
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1267
void LinearSystem::addGreaterBarrier(SolverVariable* a, SolverVariable* b, int margin, bool hasMatchConstraintWidgets) {
    if constexpr (DEBUG_CONSTRAINTS) {
        std::cout << "-> Barrier >=\n";
    }
    ArrayRow* row = createRow();
    SolverVariable* slack = createSlackVariable();
    slack->strength = 0;
    row->createRowGreaterThan(a, b, slack, margin);
    addConstraint(row);
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1289
void LinearSystem::addLowerThan(SolverVariable* a, SolverVariable* b, int margin, int strength) {
    if constexpr (DEBUG_CONSTRAINTS) {
        std::cout << "-> addLowerThan\n";
    }
    ArrayRow* row = createRow();
    SolverVariable* slack = createSlackVariable();
    slack->strength = 0;
    row->createRowLowerThan(a, b, slack, margin);
    if (strength != SolverVariable::STRENGTH_FIXED) {
        float slackValue = row->variables->get(slack);
        addSingleError(row, (int) (-1 * slackValue), strength);
    }
    addConstraint(row);
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1306
void LinearSystem::addLowerBarrier(SolverVariable* a, SolverVariable* b, int margin, bool hasMatchConstraintWidgets) {
    if constexpr (DEBUG_CONSTRAINTS) {
        std::cout << "-> Barrier <=\n";
    }
    ArrayRow* row = createRow();
    SolverVariable* slack = createSlackVariable();
    slack->strength = 0;
    row->createRowLowerThan(a, b, slack, margin);
    addConstraint(row);
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1332
void LinearSystem::addCentering(SolverVariable* a, SolverVariable* b, int m1, float bias, SolverVariable* c, SolverVariable* d, int m2, int strength) {
    if constexpr (DEBUG_CONSTRAINTS) {
        std::cout << "-> addCentering\n";
    }
    ArrayRow* row = createRow();
    row->createRowCentering(a, b, m1, bias, c, d, m2);
    if (strength != SolverVariable::STRENGTH_FIXED) {
        row->addError(this, strength);
    }
    addConstraint(row);
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1349
void LinearSystem::addRatio(SolverVariable* a, SolverVariable* b, SolverVariable* c, SolverVariable* d, float ratio, int strength) {
    if constexpr (DEBUG_CONSTRAINTS) {
        std::cout << "-> addRatio\n";
    }
    ArrayRow* row = createRow();
    row->createRowDimensionRatio(a, b, c, d, ratio);
    if (strength != SolverVariable::STRENGTH_FIXED) {
        row->addError(this, strength);
    }
    addConstraint(row);
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1368
void LinearSystem::addSynonym(SolverVariable* a, SolverVariable* b, int margin) {
    if (a->mDefinitionId == -1 && margin == 0) {
        if constexpr (DEBUG_CONSTRAINTS) {
            std::cout << "(S) -> \n";
        }
        if (b->mIsSynonym) {
            margin += (int) b->mSynonymDelta;
            b = mCache.mIndexedVariables[b->mSynonym];
        }
        if (a->mIsSynonym) {
            margin -= (int) a->mSynonymDelta;
            a = mCache.mIndexedVariables[a->mSynonym];
        } else {
            a->setSynonym(this, b, 0);
        }
    } else {
        addEquality(a, b, margin, SolverVariable::STRENGTH_FIXED);
    }
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1396
ArrayRow* LinearSystem::addEquality(SolverVariable* a, SolverVariable* b, int margin, int strength) {
    if (sMetrics != nullptr) {
        sMetrics->mSimpleEquations++;
    }
    if (USE_BASIC_SYNONYMS && strength == SolverVariable::STRENGTH_FIXED && b->isFinalValue && a->mDefinitionId == -1) {
        if constexpr (DEBUG_CONSTRAINTS) {
            std::cout << "=> Synonym\n";
        }
        a->setFinalValue(this, b->computedValue + margin);
        return nullptr;
    }
    if (DO_NOT_USE && USE_SYNONYMS && strength == SolverVariable::STRENGTH_FIXED && a->mDefinitionId == -1 && margin == 0) {
        if constexpr (DEBUG_CONSTRAINTS) {
            std::cout << "(S) -> Synonym\n";
        }
        if (b->mIsSynonym) {
            margin += (int) b->mSynonymDelta;
            b = mCache.mIndexedVariables[b->mSynonym];
        }
        if (a->mIsSynonym) {
            margin -= (int) a->mSynonymDelta;
            a = mCache.mIndexedVariables[a->mSynonym];
        } else {
            a->setSynonym(this, b, margin);
            return nullptr;
        }
    }
    if constexpr (DEBUG_CONSTRAINTS) {
        std::cout << "-> addEquality\n";
    }
    ArrayRow* row = createRow();
    row->createRowEquals(a, b, margin);
    if (strength != SolverVariable::STRENGTH_FIXED) {
        row->addError(this, strength);
    }
    addConstraint(row);
    return row;
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1446
void LinearSystem::addEquality(SolverVariable* a, int value) {
    if (sMetrics != nullptr) {
        sMetrics->mSimpleEquations++;
    }
    if (USE_BASIC_SYNONYMS && a->mDefinitionId == -1) {
        if constexpr (DEBUG_CONSTRAINTS) {
            std::cout << "=> Synonym\n";
        }
        a->setFinalValue(this, value);
        for (int i = 0; i < mVariablesID + 1; i++) {
            SolverVariable* variable = mCache.mIndexedVariables[i];
            if (variable != nullptr && variable->mIsSynonym && variable->mSynonym == a->id) {
                variable->setFinalValue(this, value + variable->mSynonymDelta);
            }
        }
        return;
    }
    if constexpr (DEBUG_CONSTRAINTS) {
        std::cout << "-> addEquality value\n";
    }
    int idx = a->mDefinitionId;
    if (a->mDefinitionId != -1) {
        ArrayRow* row = mRows[idx];
        if (row->mIsSimpleDefinition) {
            row->mConstantValue = value;
        } else {
            if (row->variables->getCurrentSize() == 0) {
                row->mIsSimpleDefinition = true;
                row->mConstantValue = value;
            } else {
                ArrayRow* newRow = createRow();
                newRow->createRowEquals(a, value);
                addConstraint(newRow);
            }
        }
    } else {
        ArrayRow* row = createRow();
        row->createRowDefinition(a, value);
        addConstraint(row);
    }
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1497
ArrayRow* LinearSystem::createRowDimensionPercent(LinearSystem* linearSystem, SolverVariable* variableA, SolverVariable* variableC, float percent) {
    if constexpr (DEBUG_CONSTRAINTS) {
        std::cout << "-> createRowDimensionPercent\n";
    }
    ArrayRow* row = linearSystem->createRow();
    return row->createRowDimensionPercent(variableA, variableC, percent);
}

// C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1515
void LinearSystem::addCenterPoint(ConstraintWidget* widget, ConstraintWidget* target, float angle, int radius) {
    SolverVariable* Al = createObjectVariable(widget->getAnchor(ConstraintAnchor::Type::LEFT));
    SolverVariable* At = createObjectVariable(widget->getAnchor(ConstraintAnchor::Type::TOP));
    SolverVariable* Ar = createObjectVariable(widget->getAnchor(ConstraintAnchor::Type::RIGHT));
    SolverVariable* Ab = createObjectVariable(widget->getAnchor(ConstraintAnchor::Type::BOTTOM));

    SolverVariable* Bl = createObjectVariable(target->getAnchor(ConstraintAnchor::Type::LEFT));
    SolverVariable* Bt = createObjectVariable(target->getAnchor(ConstraintAnchor::Type::TOP));
    SolverVariable* Br = createObjectVariable(target->getAnchor(ConstraintAnchor::Type::RIGHT));
    SolverVariable* Bb = createObjectVariable(target->getAnchor(ConstraintAnchor::Type::BOTTOM));

    ArrayRow* row = createRow();
    float angleComponent = (float) (std::sin(angle) * radius);
    row->createRowWithAngle(At, Ab, Bt, Bb, angleComponent);
    addConstraint(row);
    row = createRow();
    angleComponent = (float) (std::cos(angle) * radius);
    row->createRowWithAngle(Al, Ar, Bl, Br, angleComponent);
    addConstraint(row);
}

}

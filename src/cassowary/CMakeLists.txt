cmake_minimum_required(VERSION 3.20)

add_library(cassowary STATIC
    SolverVariable.cpp
    ArrayRow.cpp
    ArrayLinkedVariables.cpp
    SolverVariableValues.cpp
    GoalRow.cpp
    PriorityGoalRow.cpp
    LinearSystem.cpp
    ConstraintAnchor.cpp
    ConstraintWidget.cpp
    WidgetContainer.cpp
    ConstraintWidgetContainer.cpp
    Guideline.cpp
    HelperWidget.cpp
    Barrier.cpp
    ChainHead.cpp
    Chain.cpp
)

target_include_directories(cassowary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(cassowary PUBLIC cxx_std_20)
target_compile_options(cassowary PRIVATE "/utf-8")


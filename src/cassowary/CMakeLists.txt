cmake_minimum_required(VERSION 3.20)

add_library(cassowary STATIC
    SolverVariable.cpp
    ArrayRow.cpp
    ArrayLinkedVariables.cpp
    SolverVariableValues.cpp
    GoalRow.cpp
    PriorityGoalRow.cpp
    LinearSystem.cpp
)

target_include_directories(cassowary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(cassowary PUBLIC cxx_std_20)
target_compile_options(cassowary PRIVATE "/utf-8")

# Step 3: Standalone unit test for Tier 1 solver
add_executable(cassowary_test cassowary_test.cpp)
target_link_libraries(cassowary_test PRIVATE cassowary)
target_compile_options(cassowary_test PRIVATE "/utf-8")

add_executable(cassowary_debug cassowary_debug.cpp)
target_link_libraries(cassowary_debug PRIVATE cassowary)
target_compile_options(cassowary_debug PRIVATE "/utf-8")

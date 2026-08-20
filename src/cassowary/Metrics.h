// Metrics.h — Ported from androidx.constraintlayout.core.Metrics (Metrics.java)
// Original: Metrics.java:1-175
//
// Pure data struct of long counters used for solver profiling/telemetry.
// Ported as a plain struct with all public fields, matching the Java original.
//
// Copyright (C) 2018 The Android Open Source Project — Apache 2.0

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace setu::cassowary {

/// Ported from Metrics.java:23 — public class Metrics
/// Utility struct to track metrics during the system resolution.
struct Metrics {
    // Metrics.java:24-69 — all public fields
    int64_t measuresWidgetsDuration = 0;
    int64_t measuresLayoutDuration = 0;
    int64_t measuredWidgets = 0;
    int64_t measuredMatchWidgets = 0;
    int64_t measures = 0;
    int64_t additionalMeasures = 0;
    int64_t resolutions = 0;
    int64_t tableSizeIncrease = 0;
    int64_t minimize = 0;
    int64_t constraints = 0;
    int64_t simpleconstraints = 0;
    int64_t optimize = 0;
    int64_t iterations = 0;
    int64_t pivots = 0;
    int64_t bfs = 0;
    int64_t variables = 0;
    int64_t errors = 0;
    int64_t slackvariables = 0;
    int64_t extravariables = 0;
    int64_t maxTableSize = 0;
    int64_t fullySolved = 0;
    int64_t graphOptimizer = 0;
    int64_t graphSolved = 0;
    int64_t linearSolved = 0;
    int64_t resolvedWidgets = 0;
    int64_t minimizeGoal = 0;
    int64_t maxVariables = 0;
    int64_t maxRows = 0;
    int64_t nonresolvedWidgets = 0;
    std::vector<std::string> problematicLayouts;
    int64_t lastTableSize = 0;
    int64_t widgets = 0;
    int64_t measuresWrap = 0;
    int64_t measuresWrapInfeasible = 0;
    int64_t infeasibleDetermineGroups = 0;
    int64_t determineGroups = 0;
    int64_t layouts = 0;
    int64_t grouping = 0;
    int mNumberOfLayouts = 0;
    int mNumberOfMeasures = 0;
    int64_t mMeasureDuration = 0;
    int64_t mChildCount = 0;
    int64_t mMeasureCalls = 0;
    int64_t mSolverPasses = 0;
    int64_t mEquations = 0;
    int64_t mVariables = 0;
    int64_t mSimpleEquations = 0;

    /// Metrics.java:88 — public void reset()
    void reset() {
        measures = 0;
        widgets = 0;
        additionalMeasures = 0;
        resolutions = 0;
        tableSizeIncrease = 0;
        maxTableSize = 0;
        lastTableSize = 0;
        maxVariables = 0;
        maxRows = 0;
        minimize = 0;
        minimizeGoal = 0;
        constraints = 0;
        simpleconstraints = 0;
        optimize = 0;
        iterations = 0;
        pivots = 0;
        bfs = 0;
        variables = 0;
        errors = 0;
        slackvariables = 0;
        extravariables = 0;
        fullySolved = 0;
        graphOptimizer = 0;
        graphSolved = 0;
        resolvedWidgets = 0;
        nonresolvedWidgets = 0;
        linearSolved = 0;
        problematicLayouts.clear();
        mNumberOfMeasures = 0;
        mNumberOfLayouts = 0;
        measuresWidgetsDuration = 0;
        measuresLayoutDuration = 0;
        mChildCount = 0;
        mMeasureDuration = 0;
        mMeasureCalls = 0;
        mSolverPasses = 0;
        mVariables = 0;
        mEquations = 0;
        mSimpleEquations = 0;
    }

    /// Metrics.java:134 — public void copy(Metrics metrics)
    void copy(const Metrics& other) {
        mVariables = other.mVariables;
        mEquations = other.mEquations;
        mSimpleEquations = other.mSimpleEquations;
        mNumberOfMeasures = other.mNumberOfMeasures;
        mNumberOfLayouts = other.mNumberOfLayouts;
        mMeasureDuration = other.mMeasureDuration;
        mChildCount = other.mChildCount;
        mMeasureCalls = other.mMeasureCalls;
        measuresWidgetsDuration = other.measuresWidgetsDuration;
        mSolverPasses = other.mSolverPasses;
        measuresLayoutDuration = other.measuresLayoutDuration;
        measures = other.measures;
        widgets = other.widgets;
        additionalMeasures = other.additionalMeasures;
        resolutions = other.resolutions;
        tableSizeIncrease = other.tableSizeIncrease;
        maxTableSize = other.maxTableSize;
        lastTableSize = other.lastTableSize;
        maxVariables = other.maxVariables;
        maxRows = other.maxRows;
        minimize = other.minimize;
        minimizeGoal = other.minimizeGoal;
        constraints = other.constraints;
        simpleconstraints = other.simpleconstraints;
        optimize = other.optimize;
        iterations = other.iterations;
        pivots = other.pivots;
        bfs = other.bfs;
        variables = other.variables;
        errors = other.errors;
        slackvariables = other.slackvariables;
        extravariables = other.extravariables;
        fullySolved = other.fullySolved;
        graphOptimizer = other.graphOptimizer;
        graphSolved = other.graphSolved;
        resolvedWidgets = other.resolvedWidgets;
        nonresolvedWidgets = other.nonresolvedWidgets;
    }

    /// Metrics.java:74 — toString()
    std::string toString() const {
        std::string s;
        s += "\n*** Metrics ***\n";
        s += "measures: " + std::to_string(measures) + "\n";
        s += "measuresWrap: " + std::to_string(measuresWrap) + "\n";
        s += "measuresWrapInfeasible: " + std::to_string(measuresWrapInfeasible) + "\n";
        s += "determineGroups: " + std::to_string(determineGroups) + "\n";
        s += "infeasibleDetermineGroups: " + std::to_string(infeasibleDetermineGroups) + "\n";
        s += "graphOptimizer: " + std::to_string(graphOptimizer) + "\n";
        s += "widgets: " + std::to_string(widgets) + "\n";
        s += "graphSolved: " + std::to_string(graphSolved) + "\n";
        s += "linearSolved: " + std::to_string(linearSolved) + "\n";
        return s;
    }
};

} // namespace setu::cassowary

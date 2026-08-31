/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace setu {
namespace graphics {

enum class ShaderType {
    LINEAR_GRADIENT,
    RADIAL_GRADIENT,
    SWEEP_GRADIENT,
    BITMAP
};

enum class TileMode {
    CLAMP,
    REPEAT,
    MIRROR
};

class Shader {
public:
    virtual ~Shader() = default;
    virtual ShaderType getType() const = 0;
};

using ShaderPtr = std::shared_ptr<Shader>;

class LinearGradient : public Shader {
public:
    LinearGradient(float x0, float y0, float x1, float y1, 
                   const std::vector<uint32_t>& colors, 
                   const std::vector<float>& positions, 
                   TileMode tileMode)
        : mX0(x0), mY0(y0), mX1(x1), mY1(y1), 
          mColors(colors), mPositions(positions), mTileMode(tileMode) {}

    ShaderType getType() const override { return ShaderType::LINEAR_GRADIENT; }

    float mX0, mY0, mX1, mY1;
    std::vector<uint32_t> mColors;
    std::vector<float> mPositions; // Can be empty if colors are evenly distributed
    TileMode mTileMode;
};

class RadialGradient : public Shader {
public:
    RadialGradient(float centerX, float centerY, float radius,
                   const std::vector<uint32_t>& colors,
                   const std::vector<float>& positions,
                   TileMode tileMode)
        : mCenterX(centerX), mCenterY(centerY), mRadius(radius),
          mColors(colors), mPositions(positions), mTileMode(tileMode) {}

    ShaderType getType() const override { return ShaderType::RADIAL_GRADIENT; }

    float mCenterX, mCenterY, mRadius;
    std::vector<uint32_t> mColors;
    std::vector<float> mPositions;
    TileMode mTileMode;
};

class SweepGradient : public Shader {
public:
    SweepGradient(float centerX, float centerY,
                  const std::vector<uint32_t>& colors,
                  const std::vector<float>& positions)
        : mCenterX(centerX), mCenterY(centerY),
          mColors(colors), mPositions(positions) {}

    ShaderType getType() const override { return ShaderType::SWEEP_GRADIENT; }

    float mCenterX, mCenterY;
    std::vector<uint32_t> mColors;
    std::vector<float> mPositions;
};

} // namespace graphics
} // namespace setu

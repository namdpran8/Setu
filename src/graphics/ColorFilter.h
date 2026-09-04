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
#include <memory>

namespace setu {
namespace graphics {

enum class BlendMode {
    SRC_IN,
    SRC_ATOP,
    SRC_OVER,
    MULTIPLY
};

enum class ColorFilterType {
    PORTER_DUFF
};

class ColorFilter {
public:
    virtual ~ColorFilter() = default;
    virtual ColorFilterType getType() const = 0;
};

using ColorFilterPtr = std::shared_ptr<ColorFilter>;

class PorterDuffColorFilter : public ColorFilter {
public:
    PorterDuffColorFilter(uint32_t color, BlendMode mode)
        : mColor(color), mMode(mode) {}

    ColorFilterType getType() const override { return ColorFilterType::PORTER_DUFF; }

    uint32_t getColor() const { return mColor; }
    BlendMode getMode() const { return mMode; }

private:
    uint32_t mColor;
    BlendMode mMode;
};

} // namespace graphics
} // namespace setu

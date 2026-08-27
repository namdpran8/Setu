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
#include "androidfw/AssetManager2.h"
#include "../dex/ResourceManager.h"

namespace setu {

class Theme {
public:
    Theme(ResourceManager* resManager);
    ~Theme();

    // Applies a style bag to this theme.
    // If force is true, it overrides existing attributes. If false, existing attributes are kept.
    void applyStyle(uint32_t styleResId, bool force);

    android::Theme* getTheme() const { return m_theme.get(); }

private:
    ResourceManager* m_resManager;
    std::unique_ptr<android::Theme> m_theme;
};

} // namespace setu

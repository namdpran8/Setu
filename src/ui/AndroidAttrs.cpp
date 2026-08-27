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

#include "AndroidAttrs.h"

#include <unordered_map>

#include "../dex/ResourceManager.h"
#include "../utils/Logger.h"

namespace setu {

uint32_t androidAttr(ResourceManager* resManager, const std::string& name) {
    // Cache misses too: a name that does not resolve will not start resolving
    // later, and re-asking would mean a full package walk per attribute per
    // element.
    static std::unordered_map<std::string, uint32_t> cache;

    auto it = cache.find(name);
    if (it != cache.end()) return it->second;

    uint32_t resId = 0;
    if (resManager && resManager->getAssetManager()) {
        auto found = resManager->getAssetManager()->GetResourceId("android:attr/" + name);
        if (found.has_value()) {
            resId = found.value();
        } else {
            Logger::w("AndroidAttrs", "Could not resolve android:attr/" + name);
        }
    }

    cache.emplace(name, resId);
    return resId;
}

} // namespace setu

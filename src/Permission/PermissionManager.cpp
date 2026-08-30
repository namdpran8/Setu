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


#include "PermissionManager.h"
#include "../utils/Logger.h"

PermissionManager& PermissionManager::instance() {
    static PermissionManager inst;
    return inst;
}

int PermissionManager::checkPermission(const std::string& permission, int pid, int uid) const {
    Logger::i("PermissionManager", "checkPermission requested for: " + permission + ". Policy: GRANTED by default.");
    return 0; // PERMISSION_GRANTED
}

int PermissionManager::checkSelfPermission(const std::string& permission, const std::string& packageName) const {
    Logger::i("PermissionManager", "checkSelfPermission requested for: " + permission + ". Policy: GRANTED by default.");
    return 0; // PERMISSION_GRANTED
}

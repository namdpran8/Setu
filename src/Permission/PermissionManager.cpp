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
    if (permission.empty()) {
        return -1; // PERMISSION_DENIED
    }
    
    // In AOSP, this usually routes to ActivityManagerService::checkComponentPermission
    // which checks against the application's granted permissions list from PackageManager.
    // For Setu, we simulate a system where the app holds all declared permissions,
    // but we log the check for auditing.
    Logger::i("PermissionManager", "checkPermission requested for: " + permission + " (pid=" + std::to_string(pid) + ", uid=" + std::to_string(uid) + "). Policy: GRANTED by default.");
    return 0; // PERMISSION_GRANTED
}

int PermissionManager::checkSelfPermission(const std::string& permission, const std::string& packageName) const {
    if (permission.empty()) {
        return -1; // PERMISSION_DENIED
    }
    Logger::i("PermissionManager", "checkSelfPermission requested for: " + permission + ". Policy: GRANTED by default.");
    return 0; // PERMISSION_GRANTED
}

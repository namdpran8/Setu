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


// PermissionManager.h
#pragma once
#include <string>
class PermissionManager {
public:
    static PermissionManager& instance();

    int checkPermission(const std::string& permission,
        int pid, int uid) const;

    int checkSelfPermission(const std::string& permission,
        const std::string& packageName) const;

private:
    // later: per-package grant map, manifest-declared permission sets
};
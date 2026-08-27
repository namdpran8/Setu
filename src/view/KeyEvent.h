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

namespace setu {
namespace view {

class KeyEvent {
public:
    enum class Action { DOWN, UP };

    KeyEvent(Action action, int keyCode, wchar_t character = 0)
        : mAction(action), mKeyCode(keyCode), mCharacter(character) {}

    Action getAction() const { return mAction; }
    int getKeyCode() const { return mKeyCode; }
    wchar_t getCharacter() const { return mCharacter; }

private:
    Action mAction;
    int mKeyCode;
    wchar_t mCharacter;
};

} // namespace view
} // namespace setu

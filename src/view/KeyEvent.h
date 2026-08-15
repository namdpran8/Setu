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

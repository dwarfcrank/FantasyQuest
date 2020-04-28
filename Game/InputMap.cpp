#include "pch.h"
#include "InputMap.h"

void InputMap::handleEvent(const SDL_KeyboardEvent& keyEvent) const
{
    auto it = m_keys.find(keyEvent.keysym.sym); 

    if (it == m_keys.cend()) {
        return;
    }

    if (keyEvent.type == SDL_KEYDOWN) {
        it->second.handleDown();
    } else if (keyEvent.type == SDL_KEYUP) {
        it->second.handleUp();
    }
}

KeyHandler& InputMap::key(SDL_Keycode key)
{
    return m_keys[key];
}

void InputMap::unbind(SDL_Keycode key)
{
    m_keys.erase(key);
}

#include "pch.h"

#include "InputMap.h"

bool InputMap::handleEvent(const SDL_KeyboardEvent& keyEvent) const
{
    auto it = m_keys.find(keyEvent.keysym.sym); 

    if (it == m_keys.cend()) {
        return false;
    }

    if (keyEvent.type == SDL_KEYDOWN) {
        it->second.handleDown();
    } else if (keyEvent.type == SDL_KEYUP) {
        it->second.handleUp();
    } else {
        return false;
    }

    return true;
}

bool InputMap::handleEvent(const SDL_MouseMotionEvent& event) const
{
    if (m_mouseMotionHandler) {
        m_mouseMotionHandler(event);
        return true;
    }

    return false;
}

bool InputMap::handleEvent(const SDL_MouseButtonEvent& event) const
{
    auto it = m_mouseButtons.find(event.button);

    if (it == m_mouseButtons.cend()) {
        return false;
    }

    if (event.type == SDL_MOUSEBUTTONDOWN) {
        it->second.handleDown(event.x, event.y);
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        if (event.clicks == 2) {
            it->second.handleDoubleClick(event.x, event.y);
        } else {
            it->second.handleUp(event.x, event.y);
        }
    } else {
        return false;
    }

    return true;
}

KeyHandler& InputMap::key(SDL_Keycode key)
{
    return m_keys[key];
}

void InputMap::unbind(SDL_Keycode key)
{
    m_keys.erase(key);
}

void InputMap::onMouseMove(MouseMotionHandler handler)
{
    m_mouseMotionHandler = handler;
}

MouseButtonHandler& InputMap::onMouseButtons()
{
    return m_mouseButtonHandler;
}

MouseButtonHandler& InputMap::mouseButton(u8 button)
{
    return m_mouseButtons[button];
}

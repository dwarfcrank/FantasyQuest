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
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        m_mouseButtonHandler.handleDown(event.button, event.x, event.y);
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        m_mouseButtonHandler.handleUp(event.button, event.x, event.y);
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

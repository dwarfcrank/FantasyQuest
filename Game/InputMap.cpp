#include "pch.h"
#include "InputMap.h"

#include <SDL2/SDL_events.h>

void InputMap::handleEvent(const SDL_Event& event) const
{
    if (auto it = m_handlers.find(event.key.keysym.sym); it != m_handlers.cend()) {
        it->second(event.key.state == SDL_PRESSED);
    }
}

void InputMap::bind(SDL_Keycode key, InputEventHandler handler)
{
    m_handlers.emplace(key, std::move(handler));
}

void InputMap::unbind(SDL_Keycode key)
{
    m_handlers.erase(key);
}

#pragma once

#include "Common.h"

#include <SDL2/SDL_keycode.h>
#include <unordered_map>
#include <functional>

using InputEventHandler = std::function<void(bool)>;

class InputMap
{
public:
    void handleEvent(const union SDL_Event&) const;

    void bind(SDL_Keycode, InputEventHandler);
    void unbind(SDL_Keycode);

private:
    std::unordered_map<SDL_Keycode, InputEventHandler> m_handlers;
};


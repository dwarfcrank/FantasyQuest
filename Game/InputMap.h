#pragma once

#include "Common.h"

#include <SDL2/SDL_keyboard.h>
#include <unordered_map>
#include <functional>

class KeyHandler
{
public:
    using Callback = std::function<void()>;

    KeyHandler() = default;

    KeyHandler& up(Callback cb)
    {
        m_up = std::move(cb);
        return *this;
    }

    KeyHandler& down(Callback cb)
    {
        m_down = std::move(cb);
        return *this;
    }

    void handleUp() const
    {
        if (m_up) {
            m_up();
        }
    }

    void handleDown() const
    {
        if (m_down) {
            m_down();
        }
    }

private:
    Callback m_up;
    Callback m_down;
};

class InputMap
{
public:
    void handleEvent(const SDL_KeyboardEvent&) const;

    KeyHandler& key(SDL_Keycode);
    void unbind(SDL_Keycode);

private:
    std::unordered_map<SDL_Keycode, KeyHandler> m_keys;
};


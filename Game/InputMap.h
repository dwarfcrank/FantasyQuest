#pragma once

#include "Common.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <absl/container/flat_hash_map.h>
#include <functional>

using MouseMotionHandler = std::function<void(const SDL_MouseMotionEvent&)>;

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

class MouseButtonHandler
{
public:
    using Callback = std::function<void(u32, int, int)>;

    MouseButtonHandler() = default;

    MouseButtonHandler& up(Callback cb)
    {
        m_up = std::move(cb);
        return *this;
    }

    MouseButtonHandler& down(Callback cb)
    {
        m_down = std::move(cb);
        return *this;
    }

    void handleUp(u32 buttons, int x, int y) const
    {
        if (m_up) {
            m_up(buttons, x, y);
        }
    }

    void handleDown(u32 buttons, int x, int y) const
    {
        if (m_down) {
            m_down(buttons, x, y);
        }
    }

private:
    Callback m_up;
    Callback m_down;
};

class InputMap
{
public:
    bool handleEvent(const SDL_KeyboardEvent&) const;
    bool handleEvent(const SDL_MouseMotionEvent&) const;
    bool handleEvent(const SDL_MouseButtonEvent&) const;

    KeyHandler& key(SDL_Keycode);
    void unbind(SDL_Keycode);

    void onMouseMove(MouseMotionHandler);
    MouseButtonHandler& onMouseButtons();

private:
    absl::flat_hash_map<SDL_Keycode, KeyHandler> m_keys;
    MouseMotionHandler m_mouseMotionHandler;
    MouseButtonHandler m_mouseButtonHandler;
};


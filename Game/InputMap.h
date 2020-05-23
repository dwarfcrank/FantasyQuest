#pragma once

#include "Common.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <unordered_map>
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
    using Callback = std::function<void(int, int)>;

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

    MouseButtonHandler& doubleClick(Callback cb)
    {
        m_doubleClick = std::move(cb);
        return *this;
    }

    void handleUp(int x, int y) const
    {
        if (m_up) {
            m_up(x, y);
        }
    }

    void handleDown(int x, int y) const
    {
        if (m_down) {
            m_down(x, y);
        }
    }

    void handleDoubleClick(int x, int y) const
    {
        if (m_doubleClick) {
            m_doubleClick(x, y);
        }
    }

private:
    Callback m_up;
    Callback m_down;
    Callback m_doubleClick;
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

    MouseButtonHandler& mouseButton(u8 button);

private:
    std::unordered_map<SDL_Keycode, KeyHandler> m_keys;
    std::unordered_map<u8, MouseButtonHandler> m_mouseButtons;
    MouseMotionHandler m_mouseMotionHandler;
    MouseButtonHandler m_mouseButtonHandler;
};


#include "framework.h"
#include "Game.h"

#include <SDL2/SDL.h>
#include <fmt/format.h>
#include <type_traits>

template<typename... TArgs>
void reportError(const char* message, TArgs&&... args)
{
    auto msg = fmt::format(message, std::forward<TArgs>(args)...);
    MessageBoxA(nullptr, msg.c_str(), "fuck", MB_OK);
}

int main(int argc, char* argv[])
{
    if (auto ret = SDL_Init(SDL_INIT_VIDEO); ret < 0) {
        reportError("SDL_Init returned {}", ret);
        return 0;
    }

    auto window = SDL_CreateWindow("ebin", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN);
    if (!window) {
        reportError("SDL_CreateWindow returned nullptr");
        SDL_Quit();
        return 0;
    }

    auto surface = SDL_GetWindowSurface(window);

    for (int i = 0; i < 0x18; i++) {
        SDL_FillRect(surface, nullptr, SDL_MapRGB(surface->format, i, i * 2, i * 3));
        SDL_UpdateWindowSurface(window);
        SDL_Delay(20);
    }

    SDL_Delay(2000);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
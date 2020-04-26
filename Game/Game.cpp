#include "pch.h"
#include "framework.h"
#include "Game.h"

#include <SDL2/SDL.h>
#include <fmt/format.h>
#include <type_traits>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl.h>

#include "Renderer.h"

template<typename... TArgs>
void reportError(const char* message, TArgs&&... args)
{
    auto msg = fmt::format(message, std::forward<TArgs>(args)...);
    MessageBoxA(nullptr, msg.c_str(), "fuck", MB_OK);
}

using Microsoft::WRL::ComPtr;

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

    {
        Renderer r(window);

        auto triangle = r.createRenderable(std::vector{
            Vertex{.Position{ 0.0f, 0.5f, 0.5f }, .Color{ 1.0f, 0.0f, 0.0f, 1.0f }},
            Vertex{.Position{ 0.5f, -0.5f, 0.5f }, .Color{ 0.0f, 1.0f, 0.0f, 1.0f }},
            Vertex{.Position{ -0.5f, -0.5f, 0.5f }, .Color{ 0.0f, 0.0f, 1.0f, 1.0f }},
        });

        auto triangle2 = r.createRenderable(std::vector{
            Vertex{.Position{ 0.0f, 0.3f, 0.5f }, .Color{ 0.0f, 0.0f, 0.0f, 1.0f }},
            Vertex{.Position{ 0.3f, -0.3f, 0.5f }, .Color{ 0.5f, 0.5f, 0.5f, 1.0f }},
            Vertex{.Position{ -0.3f, -0.3f, 0.5f }, .Color{ 1.0f, 1.0f, 1.0f, 1.0f }},
        });

        for (int i = 0; i < 100; i++) {
            auto fi = static_cast<float>(i) / 100.0f;
            r.clear(fi * 0.1f, fi * 0.2f, fi * 0.3f);
            r.draw(triangle);
            r.draw(triangle2);
            r.endFrame();
            SDL_Delay(20);
        }

        SDL_Delay(2000);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#include "pch.h"
#include "framework.h"
#include "Game.h"
#include "Common.h"
#include "InputMap.h"

#include <SDL2/SDL.h>
#include <fmt/format.h>
#include <type_traits>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <chrono>

#include "Renderer.h"
#include "Transform.h"

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

        auto cube = r.createRenderable(
            std::vector<Vertex>{
                { XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
                { XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT4( 0.0f, 1.0f, 0.0f, 1.0f ) },
                { XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT4( 0.0f, 1.0f, 1.0f, 1.0f ) },
                { XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT4( 1.0f, 0.0f, 0.0f, 1.0f ) },
                { XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT4( 1.0f, 0.0f, 1.0f, 1.0f ) },
                { XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT4( 1.0f, 1.0f, 0.0f, 1.0f ) },
                { XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f ) },
                { XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT4( 0.0f, 0.0f, 0.0f, 1.0f ) },
            },
            std::vector<u16>{
                3, 1, 0, 2, 1, 3,
                0, 5, 4, 1, 5, 0,
                3, 4, 7, 0, 4, 3,
                1, 6, 5, 2, 6, 1,
                2, 7, 6, 3, 7, 2,
                6, 4, 5, 7, 4, 6,
            }
        );

        Camera cam;
        Transform t, t2;

        bool running = true;

        float moveSpeed = 10.0f;
        float dt = 0.0f;
        auto t0 = std::chrono::high_resolution_clock::now();

        InputMap inputs;

        inputs.bind(SDLK_a, [&](bool p) { if (p) { t.move(-moveSpeed * dt, 0.0f, 0.0f); } });
        inputs.bind(SDLK_d, [&](bool p) { if (p) { t.move(moveSpeed * dt, 0.0f, 0.0f); } });

        inputs.bind(SDLK_w, [&](bool p) { if (p) { t.move(0.0f, 0.0f, moveSpeed * dt); } });
        inputs.bind(SDLK_s, [&](bool p) { if (p) { t.move(0.0f, 0.0f, -moveSpeed * dt); } });

        inputs.bind(SDLK_ESCAPE, [&](bool) { running = false; });

        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_KEYUP:
                case SDL_KEYDOWN:
                    inputs.handleEvent(event);
                    break;

                default:
                    break;
                }
            }

            r.clear(0.1f, 0.2f, 0.3f);
            r.draw(cube, cam, t);
            r.endFrame();

            auto t1 = std::chrono::high_resolution_clock::now();
            auto dtMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
            dt = static_cast<float>(dtMs.count()) / 1000.0f;
            t0 = t1;
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
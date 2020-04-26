#include "pch.h"
#include "framework.h"
#include "Game.h"
#include "Common.h"

#include <SDL2/SDL.h>
#include <fmt/format.h>
#include <type_traits>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl.h>

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
        Transform t;

        for (int i = 0; i < 100; i++) {
            auto fi = static_cast<float>(i) / 100.0f;
            t.rotate(0.0f, 0.05f, 0.0f);
            r.clear(fi * 0.1f, fi * 0.2f, fi * 0.3f);
            r.draw(cube, cam, t);
            r.endFrame();
            SDL_Delay(20);
        }

        SDL_Delay(2000);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
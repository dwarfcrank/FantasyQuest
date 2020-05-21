#include "pch.h"

#include "Game.h"
#include "Common.h"
#include "InputMap.h"
#include "Renderer.h"
#include "Transform.h"
#include "Mesh.h"
#include "File.h"
#include "Scene.h"
#include "Camera.h"
#include "Math.h"

#include <SDL2/SDL.h>
#include <fmt/format.h>
#include <type_traits>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <chrono>

using namespace math;

GameBase::GameBase(InputMap& inputs) :
    m_inputs(inputs)
{
}

Game::Game(Scene& scene, InputMap& inputs) :
    GameBase(inputs), scene(scene)
{
    auto doBind = [this](SDL_Keycode k, float& target, float value) {
        m_inputs.key(k)
            .down([&target,  value] { target = value; })
            .up([&target] { target = 0.0f; });
    };

    doBind(SDLK_q, angle, -turnSpeed);
    doBind(SDLK_e, angle, turnSpeed);

    doBind(SDLK_d, velocity.x, moveSpeed);
    doBind(SDLK_a, velocity.x, -moveSpeed);
    doBind(SDLK_w, velocity.z, moveSpeed);
    doBind(SDLK_s, velocity.z, -moveSpeed);

    doBind(SDLK_LEFT, lightVelocity.x, -moveSpeed);
    doBind(SDLK_RIGHT, lightVelocity.x, moveSpeed);
    doBind(SDLK_UP, lightVelocity.z, moveSpeed);
    doBind(SDLK_DOWN, lightVelocity.z, -moveSpeed);

    doBind(SDLK_r, lightVelocity.y, moveSpeed);
    doBind(SDLK_f, lightVelocity.y, -moveSpeed);

    m_inputs.onMouseMove([this](const SDL_MouseMotionEvent& event) {
        if ((event.state & SDL_BUTTON_RMASK) || (SDL_GetModState() & KMOD_LCTRL)) {
            auto x = static_cast<float>(event.xrel) / 450.0f;
            auto y = static_cast<float>(event.yrel) / 450.0f;
            m_camera.rotate(y, x);
        }
    });
}

bool Game::update(float dt)
{
    bool running = true;

    m_camera.move(Vector<View>(velocity.x, velocity.y, velocity.z) * dt);
    m_camera.move(Vector<World>(lightVelocity.x, lightVelocity.y, lightVelocity.z) * dt);
    m_camera.rotate(0.0f, angle * dt);

    return running;
}

void Game::render(IRenderer* r)
{
}

const Camera& Game::getCamera() const
{
    return m_camera;
}


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
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_sdl.h"
#include "Mesh.h"

template<typename... TArgs>
void reportError(const char* message, TArgs&&... args)
{
    auto msg = fmt::format(message, std::forward<TArgs>(args)...);
    MessageBoxA(nullptr, msg.c_str(), "fuck", MB_OK);
}

struct Model
{
    Model(std::string name, Renderable* renderable) :
        name{ std::move(name) }, renderable(renderable)
    {
    }

    std::string name;
    Renderable* renderable;
    int count = 0;
};

struct Object
{
    Object(const Model& model, const Transform& t) :
        renderable(model.renderable), transform(t)
    {
        name = fmt::format("{}:{}", model.name, model.count);
        XMStoreFloat3(&position, t.Position);
        XMStoreFloat3(&rotation, t.Rotation);
        XMStoreFloat3(&scale, t.Scale);
    }

    void update()
    {
        transform.Position = XMLoadFloat3(&position);
        transform.Rotation = XMLoadFloat3(&rotation);
        transform.Scale = XMLoadFloat3(&scale);
    }

    Renderable* renderable;

    std::string name;
    Transform transform;

    XMFLOAT3 position;
    XMFLOAT3 rotation;
    XMFLOAT3 scale;
};

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

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        auto& io = ImGui::GetIO();
        ImGui::StyleColorsDark();
        ImGui_ImplSDL2_InitForD3D(window);
        ImGui_ImplDX11_Init(r.getDevice(), r.getDeviceContext());

        auto cube = r.createRenderable(
            std::vector<Vertex>{
                { XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f ) },
                { XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4( 0.0f, 1.0f, 0.0f, 1.0f ) },
                { XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4( 0.0f, 1.0f, 1.0f, 1.0f ) },
                { XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4( 1.0f, 0.0f, 0.0f, 1.0f ) },
                { XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4( 1.0f, 0.0f, 1.0f, 1.0f ) },
                { XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4( 1.0f, 1.0f, 0.0f, 1.0f ) },
                { XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f ) },
                { XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4( 0.0f, 0.0f, 0.0f, 1.0f ) },
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

        Renderable* stone = nullptr;
        {
            Mesh m("../Assets/stone_tallA.fbx");
            stone = r.createRenderable(m.getVertices(), m.getIndices());
        }

        Renderable* tree = nullptr;
        {
            Mesh m("../Assets/tree_detailed.fbx");
            tree = r.createRenderable(m.getVertices(), m.getIndices());
        }

        std::vector<Object> scene;
        std::vector<Model> models;

        for (auto name : { "stone_tallA", "tree_detailed" }) {
            Mesh m(fmt::format("../Assets/{}.fbx", name));
            auto renderable = r.createRenderable(m.getVertices(), m.getIndices());
            models.emplace_back(name, renderable);
        }

        Camera cam;
        Transform t, t2;

        bool running = true;

        float moveSpeed = 5.1f;
        float turnSpeed = 3.0f;
        float angle = 0.0f;
        float velocity = 0.0f;
        float upVelocity = 0.0f;

        float dt = 0.0f;
        auto t0 = std::chrono::high_resolution_clock::now();
        const auto forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

        InputMap inputs;

        inputs.key(SDLK_a)
            .down([&] { angle = -turnSpeed * dt; })
            .up([&] { angle = 0.0f; });

        inputs.key(SDLK_d)
            .down([&] { angle = turnSpeed * dt; })
            .up([&] { angle = 0.0f; });

        inputs.key(SDLK_w)
            .down([&] { velocity = moveSpeed * dt; })
            .up([&] { velocity = 0.0f; });

        inputs.key(SDLK_s)
            .down([&] { velocity = -moveSpeed * dt; })
            .up([&] { velocity = 0.0f; });

        inputs.key(SDLK_r)
            .down([&] { upVelocity = moveSpeed * dt; })
            .up([&] { upVelocity = 0.0f; });

        inputs.key(SDLK_f)
            .down([&] { upVelocity = -moveSpeed * dt; })
            .up([&] { upVelocity = 0.0f; });

        inputs.key(SDLK_ESCAPE).up([&] { running = false; });

        std::size_t modelIdx = 0;
        inputs.key(SDLK_c).up([&] { modelIdx = (modelIdx + 1) % models.size(); });

        inputs.key(SDLK_SPACE).up([&] {
            scene.emplace_back(models[modelIdx], t);
            models[modelIdx].count++;
        });

        bool showDemo = false;
        inputs.key(SDLK_HOME).up([&] { showDemo = !showDemo; });

        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event);

                switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_KEYUP:
                case SDL_KEYDOWN:
                    inputs.handleEvent(event.key);
                    break;

                default:
                    break;
                }
            }

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplSDL2_NewFrame(window);
            ImGui::NewFrame();

            if (showDemo) {
                ImGui::ShowDemoWindow(&showDemo);
            }

            if constexpr (true) {
                ImGui::Begin("Scene");

                for (int i = 0; i < scene.size(); i++) {
                    auto& obj = scene[i];
                    auto id = reinterpret_cast<void*>(static_cast<uintptr_t>(i));

                    //if (ImGui::TreeNode(id, "Object %d", i)) {
                    if (ImGui::TreeNode(obj.name.c_str())) {
                        if (ImGui::InputFloat3("Position", &obj.position.x)) {
                            obj.update();
                        }

                        if (ImGui::InputFloat3("Rotation", &obj.rotation.x)) {
                            obj.update();
                        }

                        if (ImGui::InputFloat3("Scale", &obj.scale.x)) {
                            obj.update();
                        }

                        ImGui::TreePop();
                    }
                }

                ImGui::End();
            }

            ImGui::Render();

            r.clear(0.1f, 0.2f, 0.3f);
            t2.rotate(0.0f, angle, 0.0f);
            auto m = t2.getMatrix();
            auto d = XMVector3TransformNormal(forward, m);
            t.move(d * velocity);
            t.move(XMVectorSet(0.0f, upVelocity, 0.0f, 0.0f));
            t.Rotation = t2.Rotation;
            //r.draw(cube, cam, t);
            r.draw(models[modelIdx].renderable, cam, t);

            for (const auto& o : scene) {
                r.draw(o.renderable, cam, o.transform);
            }

            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            r.endFrame();

            auto t1 = std::chrono::high_resolution_clock::now();
            auto dtMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);
            dt = static_cast<float>(dtMs.count()) / 1000.0f;
            t0 = t1;
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
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

    auto window = SDL_CreateWindow("ebin", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, SDL_WINDOW_SHOWN);
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
                { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
                { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
                { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
                { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
                { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
                { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
                { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
                { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
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

        {
            std::filesystem::directory_iterator end;
            for (auto it = std::filesystem::directory_iterator("../Assets"); it != end; ++it) {
                if (!it->is_regular_file()) {
                    continue;
                }

                const auto p = it->path();
                if (p.extension() == ".fbx") {
                    Mesh mesh(p);
                    auto name = p.filename().replace_extension().string();

                    auto renderable = r.createRenderable(mesh.getVertices(), mesh.getIndices());
                    models.emplace_back(name, renderable);
                }
            }
        }

        std::vector<const char*> modelNames;
        for (const auto& model : models) {
            modelNames.push_back(model.name.c_str());
        }

        Camera cam;
        Transform t, t2;

        bool running = true;

        float moveSpeed = 5.1f;
        //float moveSpeed = 10.0f;
        float turnSpeed = 3.0f;
        float angle = 0.0f;

        XMFLOAT3 velocity(0.0f, 0.0f, 0.0f);

        float dt = 0.0f;
        auto t0 = std::chrono::high_resolution_clock::now();
        const auto forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

        InputMap inputs;

        inputs.key(SDLK_e)
            .down([&] { angle = -turnSpeed * dt; })
            .up([&] { angle = 0.0f; });

        inputs.key(SDLK_q)
            .down([&] { angle = turnSpeed * dt; })
            .up([&] { angle = 0.0f; });

        inputs.key(SDLK_a)
            .down([&] { velocity.x = -moveSpeed * dt; })
            .up([&] { velocity.x = 0.0f; });

        inputs.key(SDLK_d)
            .down([&] { velocity.x = moveSpeed * dt; })
            .up([&] { velocity.x = 0.0f; });

        inputs.key(SDLK_w)
            .down([&] { velocity.z = moveSpeed * dt; })
            .up([&] { velocity.z = 0.0f; });

        inputs.key(SDLK_s)
            .down([&] { velocity.z = -moveSpeed * dt; })
            .up([&] { velocity.z = 0.0f; });

        inputs.key(SDLK_r)
            .down([&] { velocity.y = moveSpeed * dt; })
            .up([&] { velocity.y = 0.0f; });

        inputs.key(SDLK_f)
            .down([&] { velocity.y = -moveSpeed * dt; })
            .up([&] { velocity.y = 0.0f; });

        inputs.key(SDLK_ESCAPE).up([&] { running = false; });

        int modelIdx = 0;

        inputs.key(SDLK_SPACE).up([&] {
            scene.emplace_back(models[modelIdx], t);
            models[modelIdx].count++;
        });

        bool showDemo = false;
        inputs.key(SDLK_HOME).up([&] { showDemo = !showDemo; });

        XMFLOAT3 tPos(0.0f, 0.0f, 0.0f);
        XMFLOAT3 tRot(0.0f, 0.0f, 0.0f);
        XMFLOAT3 tScl(0.0f, 0.0f, 0.0f);

        XMFLOAT3 light(1.0f, 1.0f, -1.0f);

        std::vector<PointLight> lights;

        PointLight lightTemplate{
            .Position{-1.0f, 0.0f, 0.0f, 1.0f},
            .Color{1.0f, 0.0f, 0.0f, 0.0f},
        };

        if constexpr (false) {
            lights.push_back(PointLight{
                /*
                .Position = XMVectorSet(0.0f, 3.0f, 1.0f, 1.0f),
                .Color = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
                */
                .Position{-1.0f, 0.0f, 0.0f, 1.0f},
                .Color{1.0f, 0.0f, 0.0f, 0.0f},
            });

            lights.push_back(PointLight{
                /*
                .Position = XMVectorSet(0.0f, -3.0f, -1.0f, 1.0f),
                .Color = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
                */
                .Position{1.0f, 0.0f, 0.0f, 1.0f},
                .Color{0.0f, 0.0f, 1.0f, 0.0f},
            });
        }
        //r.setPointLights(lights);

        while (running) {
            XMStoreFloat3(&tPos, t.Position);
            XMStoreFloat3(&tRot, t.Rotation);
            XMStoreFloat3(&tScl, t.Scale);

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

                if (ImGui::TreeNode("Lights")) {
                    if (ImGui::InputFloat3("Directional", &light.x)) {
                        r.setDirectionalLight(light);
                    }

                    ImGui::Separator();

                    for (int i = 0; i < lights.size(); i++) {
                        auto id = reinterpret_cast<void*>(static_cast<uintptr_t>(i));

                        if (ImGui::TreeNode(id, "Light %d", i)) {
                            if (ImGui::InputFloat3("Position", &lights[i].Position.x)) {
                                r.setPointLights(lights);
                            }

                            if (ImGui::ColorEdit3("Color", &lights[i].Color.x)) {
                                r.setPointLights(lights);
                            }

                            if (ImGui::SliderFloat("Linear", &lights[i].Position.w, 0.0f, 5.0f, "%f")) {
                                r.setPointLights(lights);
                            }

                            if (ImGui::SliderFloat("Quadratic", &lights[i].Color.w, 0.0f, 5.0f, "%f")) {
                                r.setPointLights(lights);
                            }

                            ImGui::TreePop();
                        }
                    }

                    if (ImGui::Button("Add light")) {
                        lights.push_back(
                            PointLight{
                                .Position{-1.0f, 0.0f, 0.0f, 1.0f},
                                .Color{1.0f, 0.0f, 0.0f, 0.0f},
                            });

                        r.setPointLights(lights);
                    }

                    ImGui::TreePop();
                }

                ImGui::Separator();

                if (ImGui::TreeNode("Objects")) {
                    for (int i = 0; i < scene.size(); i++) {
                        auto& obj = scene[i];
                        auto id = reinterpret_cast<void*>(static_cast<uintptr_t>(i));

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

                    ImGui::TreePop();
                }

                ImGui::End();
            }

            if constexpr (true) {
                ImGui::Begin("Stuff");

                ImGui::ListBox("Models", &modelIdx,
                    [](void* data, int idx, const char** output) -> bool {
                        const auto items = reinterpret_cast<Model*>(data);
                        *output = items[idx].name.c_str();
                        return true;
                    },
                    models.data(), static_cast<int>(models.size()), 8
                );

                ImGui::Separator();

                if (ImGui::InputFloat3("Position", &tPos.x)) {
                    t.Position = XMLoadFloat3(&tPos);
                }

                if (ImGui::InputFloat3("Rotation", &tRot.x)) {
                    t.Rotation = XMLoadFloat3(&tRot);
                }

                if (ImGui::InputFloat3("Scale", &tScl.x)) {
                    t.Scale = XMLoadFloat3(&tScl);
                }

                ImGui::End();
            }

            ImGui::Render();

            r.clear(0.1f, 0.2f, 0.3f);

            /*
            t2.rotate(0.0f, angle, 0.0f);
            auto m = t2.getMatrix();
            auto d = XMVector3TransformNormal(forward, m);
            t.move(d * velocity);
            t.move(XMVectorSet(0.0f, upVelocity, 0.0f, 0.0f));
            t.Rotation = t2.Rotation;
            */
            t.move(velocity.x, velocity.y, velocity.z);
            t.rotate(0.0f, angle, 0.0f);
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
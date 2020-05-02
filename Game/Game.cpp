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
#include <nlohmann/json.hpp>

#include "Renderer.h"
#include "Transform.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_sdl.h"
#include "Mesh.h"
#include "File.h"

template<typename... TArgs>
void reportError(const char* message, TArgs&&... args)
{
    auto msg = fmt::format(message, std::forward<TArgs>(args)...);
    MessageBoxA(nullptr, msg.c_str(), "fuck", MB_OK);
}

namespace nlohmann
{
    template<>
    struct adl_serializer<DirectX::XMFLOAT3>
    {
        static void to_json(json& j, const DirectX::XMFLOAT3& v)
        {
            j.push_back(v.x);
            j.push_back(v.y);
            j.push_back(v.z);
        }

        static void from_json(const json& j, DirectX::XMFLOAT3& v)
        {
            v.x = j[0].get<float>();
            v.y = j[1].get<float>();
            v.z = j[2].get<float>();
        }
    };

    template<>
    struct adl_serializer<DirectX::XMFLOAT4>
    {
        static void to_json(json& j, const DirectX::XMFLOAT4& v)
        {
            j.push_back(v.x);
            j.push_back(v.y);
            j.push_back(v.z);
            j.push_back(v.w);
        }

        static void from_json(const json& j, DirectX::XMFLOAT4& v)
        {
            v.x = j[0].get<float>();
            v.y = j[1].get<float>();
            v.z = j[2].get<float>();
            v.w = j[3].get<float>();
        }
    };
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
    Object() = default;

    Object(const Model& model, const Transform& t) :
        renderable(model.renderable), transform(t), modelName(model.name)
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

    Renderable* renderable = nullptr;

    std::string name;
    std::string modelName;
    Transform transform;

    XMFLOAT3 position;
    XMFLOAT3 rotation;
    XMFLOAT3 scale;
};

static void to_json(nlohmann::json& j, const Object& obj)
{
    j["name"] = obj.name;
    j["model"] = obj.modelName;
    j["position"] = obj.position;
    j["rotation"] = obj.rotation;
    j["scale"] = obj.scale;
}

static void from_json(const nlohmann::json& j, Object& obj)
{
    j["name"].get_to(obj.name);
    j["model"].get_to(obj.modelName);
    j["position"].get_to(obj.position);
    j["rotation"].get_to(obj.rotation);
    j["scale"].get_to(obj.scale);

    obj.update();
}

static void to_json(nlohmann::json& j, const PointLight& p)
{
    j["position"] = p.Position;
    j["color"] = p.Color;
}

static void from_json(const nlohmann::json& j, PointLight& p)
{
    j["position"].get_to(p.Position);
    j["color"].get_to(p.Color);
}

struct Scene
{
    XMFLOAT3 directionalLight{ 1.0f, 1.0f, -1.0f };

    std::vector<Object> objects;
    std::vector<PointLight> lights;
};

static void to_json(nlohmann::json& j, const Scene& s)
{
    j["directionalLight"] = s.directionalLight;
    j["lights"] = s.lights;
    j["objects"] = s.objects;
}

static void from_json(const nlohmann::json& j, Scene& s)
{
    j["directionalLight"].get_to(s.directionalLight);
    j["lights"].get_to(s.lights);
    j["objects"].get_to(s.objects);
}

std::vector<Model> models;
std::unordered_map<std::string, Renderable*> renderables;

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

        Scene scene;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        auto& io = ImGui::GetIO();
        ImGui::StyleColorsDark();
        ImGui_ImplSDL2_InitForD3D(window);
        ImGui_ImplDX11_Init(r.getDevice(), r.getDeviceContext());

        {
            std::filesystem::directory_iterator end;
            for (auto it = std::filesystem::directory_iterator("../Assets"); it != end; ++it) {
                if (!it->is_regular_file()) {
                    continue;
                }

                const auto p = it->path();
                if (p.extension() == ".fbx") {
                    Mesh mesh(p);

                    auto renderable = r.createRenderable(mesh.getVertices(), mesh.getIndices());
                    models.emplace_back(mesh.getName(), renderable);
                    renderables.emplace(mesh.getName(), renderable);
                }
            }
        }

        Camera cam;
        Transform t, t2;

        bool running = true;

        float moveSpeed = 5.1f;
        float turnSpeed = 3.0f;
        float angle = 0.0f;

        XMFLOAT3 velocity(0.0f, 0.0f, 0.0f);

        float dt = 0.0f;
        auto t0 = std::chrono::high_resolution_clock::now();
        const auto forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

        InputMap inputs;

        auto doBind = [&inputs, &dt](SDL_Keycode k, float& target, float value) {
            inputs.key(k)
                .down([&target, &dt, value] { target = value * dt; })
                .up([&target] { target = 0.0f; });
        };

        doBind(SDLK_q, angle, turnSpeed);
        doBind(SDLK_e, angle, -turnSpeed);
        doBind(SDLK_d, velocity.x, moveSpeed);
        doBind(SDLK_a, velocity.x, -moveSpeed);
        doBind(SDLK_w, velocity.z, moveSpeed);
        doBind(SDLK_s, velocity.z, -moveSpeed);
        doBind(SDLK_r, velocity.y, moveSpeed);
        doBind(SDLK_f, velocity.y, -moveSpeed);

        inputs.key(SDLK_ESCAPE).up([&] { running = false; });

        int modelIdx = 0;

        inputs.key(SDLK_SPACE).up([&] {
            scene.objects.emplace_back(models[modelIdx], t);
            models[modelIdx].count++;
        });

        bool showDemo = false;
        inputs.key(SDLK_HOME).up([&] { showDemo = !showDemo; });

        XMFLOAT3 tPos(0.0f, 0.0f, 0.0f);
        XMFLOAT3 tRot(0.0f, 0.0f, 0.0f);
        XMFLOAT3 tScl(0.0f, 0.0f, 0.0f);

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
                    if (!io.WantCaptureKeyboard) {
                        inputs.handleEvent(event.key);
                    }
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
                {
                    static char scenePath[128] = "../scene.json";
                    ImGui::InputText("Filename", scenePath, sizeof(scenePath));

                    if (ImGui::Button("Save")) {
                        nlohmann::json j;
                        j = scene;
                        std::ofstream os(scenePath);
                        os << j;
                    }

                    if (ImGui::Button("Load")) {
                        auto d = loadFile(scenePath);
                        nlohmann::json::parse(d).get_to(scene);

                        for (auto& o : scene.objects) {
                            o.renderable = renderables[o.modelName];
                        }

                        r.setDirectionalLight(scene.directionalLight);
                        r.setPointLights(scene.lights);
                    }
                }

                if (ImGui::TreeNode("Lights")) {
                    if (ImGui::InputFloat3("Directional", &scene.directionalLight.x)) {
                        r.setDirectionalLight(scene.directionalLight);
                    }

                    ImGui::Separator();

                    for (int i = 0; i < scene.lights.size(); i++) {
                        auto id = reinterpret_cast<void*>(static_cast<uintptr_t>(i));

                        if (ImGui::TreeNode(id, "Light %d", i)) {
                            bool changed = false;

                            changed |= ImGui::InputFloat3("Position", &scene.lights[i].Position.x);
                            changed |= ImGui::ColorEdit3("Color", &scene.lights[i].Color.x);
                            changed |= ImGui::SliderFloat("Linear", &scene.lights[i].Position.w, 0.0f, 5.0f, "%f");
                            changed |= ImGui::SliderFloat("Quadratic", &scene.lights[i].Color.w, 0.0f, 5.0f, "%f");

                            if (changed) {
                                r.setPointLights(scene.lights);
                            }

                            ImGui::TreePop();
                        }
                    }

                    if (ImGui::Button("Add light")) {
                        scene.lights.push_back(
                            PointLight{
                                .Position{-1.0f, 0.0f, 0.0f, 1.0f},
                                .Color{1.0f, 0.0f, 0.0f, 0.0f},
                            });

                        r.setPointLights(scene.lights);
                    }

                    ImGui::TreePop();
                }

                ImGui::Separator();

                if (ImGui::TreeNode("Objects")) {
                    for (int i = 0; i < scene.objects.size(); i++) {
                        auto& obj = scene.objects[i];
                        auto id = reinterpret_cast<void*>(static_cast<uintptr_t>(i));

                        if (ImGui::TreeNode(obj.name.c_str())) {
                            bool changed = false;

                            changed |= ImGui::InputFloat3("Position", &obj.position.x);
                            changed |= ImGui::InputFloat3("Rotation", &obj.rotation.x);
                            changed |= ImGui::InputFloat3("Scale", &obj.scale.x);

                            if (changed) {
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
            t.move(velocity.x, velocity.y, velocity.z);
            t.rotate(0.0f, angle, 0.0f);
            r.draw(models[modelIdx].renderable, cam, t);

            for (const auto& o : scene.objects) {
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
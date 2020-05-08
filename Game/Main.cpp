#include "pch.h"
#include "framework.h"
#include "Game.h"
#include "Common.h"
#include "InputMap.h"
#include "Renderer.h"
#include "Transform.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_sdl.h"
#include "Mesh.h"
#include "File.h"
#include "Scene.h"
#include "Camera.h"
#include "GameTime.h"
#include "SceneEditor.h"
#include "ArrayView.h"

#include <SDL2/SDL.h>
#include <fmt/format.h>
#include <type_traits>
#include <DirectXMath.h>
#include <chrono>

extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 1;

template<typename... TArgs>
void reportError(const char* message, TArgs&&... args)
{
    auto msg = fmt::format(message, std::forward<TArgs>(args)...);
    MessageBoxA(nullptr, msg.c_str(), "fuck", MB_OK);
}

void loadAssets(IRenderer* r, std::vector<RModel>& models, std::unordered_map<std::string, Renderable*>& renderables)
{
    std::filesystem::directory_iterator end;
    for (auto it = std::filesystem::directory_iterator("../Assets"); it != end; ++it) {
        if (!it->is_regular_file()) {
            continue;
        }

        const auto p = it->path();
        if (p.extension() == ".fbx") {
            Mesh mesh(p);

            auto a = ArrayView(mesh.getVertices());
            auto b = a.byteSize();

            auto renderable = r->createRenderable(mesh.getVertices(), mesh.getIndices());
            models.emplace_back(mesh.getName(), renderable, mesh.getBounds());
            renderables.emplace(mesh.getName(), renderable);
        }
    }
}

int main(int argc, char* argv[])
{
    if (auto ret = SDL_Init(SDL_INIT_VIDEO); ret < 0) {
        reportError("SDL_Init returned {}", ret);
        return 0;
    }

    //auto window = SDL_CreateWindow("ebin", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, SDL_WINDOW_SHOWN);
    auto window = SDL_CreateWindow("ebin", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_SHOWN);
    if (!window) {
        reportError("SDL_CreateWindow returned nullptr");
        SDL_Quit();
        return 0;
    }

    {
        auto r = createRenderer(window);

        std::vector<RModel> models;
        std::unordered_map<std::string, Renderable*> renderables;
        std::unordered_map<std::string, Bounds> bounds;

        Scene scene;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        auto& io = ImGui::GetIO();
        ImGui::StyleColorsDark();
        ImGui_ImplSDL2_InitForD3D(window);
        r->initImgui();

        loadAssets(r.get(), models, renderables);
        {
            for (const auto& model : models) {
                bounds[model.name] = model.bounds;
            }
        }

        InputMap inputs;

        bool running = true;
        inputs.key(SDLK_ESCAPE).up([&] { running = false; });

        {
            scene.load("../scene.json");

            for (auto& o : scene.objects) {
                o.renderable = renderables[o.modelName];
                o.bounds = bounds[o.modelName];
            }

            r->setDirectionalLight(scene.directionalLight, scene.directionalLightColor);
            r->setPointLights(scene.lights);
        }

        Game game(scene, inputs);
        SceneEditor editor(scene, inputs);

        std::array<GameBase*, 2> games{ &game, &editor };
        size_t gameIdx = 1;
        inputs.key(SDLK_F1).up([&] { gameIdx++; gameIdx %= games.size(); });

        GameTime gt;

        bool showDemo = false;
        inputs.key(SDLK_HOME).up([&] { showDemo = !showDemo; });

        auto handleEvents = [&io, &inputs, &running] {
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

                case SDL_MOUSEMOTION:
                    if (!io.WantCaptureMouse) {
                        inputs.handleEvent(event.motion);
                    }
                    break;

                default:
                    break;
                }
            }
        };

        Camera shadowCam = Camera::ortho();

        XMFLOAT3 shadowDir{ 0.0f, 0.0f, 0.0f };

        std::vector<DebugDrawVertex> debugVerts;

        std::unordered_map<Renderable*, RenderBatch> batches;

        for (const auto& [k, v] : renderables) {
            batches[v].renderable = v;
        }

        auto updateBatches = [&] {
            for (auto& [_, batch] : batches) {
                batch.instances.clear();
            }

            for (const auto& o : scene.objects) {
                auto wm = o.transform.getMatrix();
                auto& instance = batches[o.renderable].instances.emplace_back();
                instance.World = XMMatrixTranspose(wm);
                instance.WorldInvTranspose = XMMatrixInverse(nullptr, wm);
            }
        };

        while (running) {
            auto g = games[gameIdx];
            float dt = gt.update();

            handleEvents();

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplSDL2_NewFrame(window);
            ImGui::NewFrame();

            g->update(dt);

            if (showDemo) {
                ImGui::ShowDemoWindow(&showDemo);
            }

            ImGui::Render();

            {
                shadowCam.setRotation(scene.directionalLight.x, scene.directionalLight.y);

                auto rotation = XMQuaternionRotationRollPitchYaw(scene.directionalLight.x, scene.directionalLight.y, 0.0f);
                auto direction = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), rotation);
                XMFLOAT3 d;
                XMStoreFloat3(&d, direction);
                r->setDirectionalLight(d, scene.directionalLightColor);

                r->setPointLights(scene.lights);
            }

            r->beginShadowPass();
            {
                for (const auto& o : scene.objects) {
                    r->drawShadow(o.renderable, shadowCam, o.transform);
                }
            }
            r->endShadowPass();

            r->beginFrame();
            {
                r->clear(0.1f, 0.2f, 0.3f);

                /*
                for (const auto& o : scene.objects) {
                    r->draw(o.renderable, g->getCamera(), o.transform);
                }
                */
                updateBatches();
                for (const auto& [_, batch] : batches) {
                    r->draw(batch, g->getCamera());
                }

                g->render(r.get());

                r->postProcess();

                if (auto drawData = ImGui::GetDrawData()) {
                    ImGui_ImplDX11_RenderDrawData(drawData);
                }
            }
            r->endFrame();
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

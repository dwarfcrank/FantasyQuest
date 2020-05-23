#include "pch.h"
#include "framework.h"
#include "Game.h"
#include "Common.h"
#include "InputMap.h"
#include "Renderer.h"
#include "Transform.h"
#include "Mesh.h"
#include "File.h"
#include "Scene.h"
#include "Camera.h"
#include "GameTime.h"
#include "SceneEditor.h"
#include "ArrayView.h"

#include "PhysicsWorld.h"
#include "Components/Transform.h"
#include "Components/Renderable.h"
#include "Components/PointLight.h"

#include <im3d.h>
#include <SDL2/SDL.h>
#include <fmt/format.h>
#include <type_traits>
#include <DirectXMath.h>
#include <chrono>
#include <entt/entt.hpp>
#include <random>

#include <unordered_map>
#include <unordered_set>

#include <bullet/btBulletDynamicsCommon.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_sdl.h>

using namespace math;

extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 1;

template<typename... TArgs>
void reportError(const char* message, TArgs&&... args)
{
    auto msg = fmt::format(message, std::forward<TArgs>(args)...);
    MessageBoxA(nullptr, msg.c_str(), "fuck", MB_OK);
}

std::vector<ModelAsset> loadModels(IRenderer* r)
{
    std::filesystem::directory_iterator end;

    std::vector<ModelAsset> models;

    for (auto it = std::filesystem::directory_iterator("./content"); it != end; ++it) {
        if (!it->is_regular_file()) {
            continue;
        }

        const auto p = it->path();

        if (p.extension() == ".mesh") {
            auto mesh = Mesh::load(p);
            auto renderable = r->createRenderable(mesh.getName(), mesh.getVertices(), mesh.getIndices());
            models.emplace_back(mesh.getName(), renderable, mesh.getBounds(), p.generic_string());
        }
    }

    return models;
}

void convertAssets(const std::filesystem::path& in, const std::filesystem::path& out)
{
    std::filesystem::directory_iterator end;
    std::filesystem::create_directories(out);
    std::unordered_set<std::string> meshNames;

    for (auto it = std::filesystem::directory_iterator(in); it != end; ++it) {
        if (!it->is_regular_file()) {
            continue;
        }

        const auto p = it->path();
        if (p.extension() == ".fbx") {
            auto mesh = Mesh::import(p);

            auto meshName = mesh.getName();

            int i = 0;

            while (meshNames.contains(meshName)) {
                meshName = fmt::format("{}#{}", mesh.getName(), i);
                i++;
            }

            mesh.setName(meshName);
            meshNames.insert(meshName);

            auto pOut = p.filename();
            pOut.replace_extension(".mesh");

            Mesh::save(out/pOut, mesh);
        }
    }
}

std::vector<std::string_view> getArgs(int argc, char* argv[])
{
    std::vector<std::string_view> args;

    for (int i = 0; i < argc; i++) {
        args.emplace_back(argv[i]);
    }

    return args;
}

class MainLoop
{
public:
    MainLoop(SDL_Window* window, const std::filesystem::path& scenePath = {});
    ~MainLoop();

    void handleEvents();
    void update();
    bool isRunning() const { return m_running; }

private:
    void updateLights();
    void updateBatches();

    std::vector<PointLight> m_lights;

    std::unique_ptr<IRenderer> m_renderer;
    InputMap m_inputs;
    bool m_running = true;
    std::vector<ModelAsset> m_models;
    SDL_Window* m_window = nullptr;

    Scene m_scene;

    std::unique_ptr<Game> game;
    std::unique_ptr<SceneEditor> editor;

    std::array<GameBase*, 2> m_games;
    size_t m_gameIdx = 1;

    GameTime m_gameTime;
    bool m_showDemo = false;

    XMFLOAT2 m_mouse{ 0.0f, 0.0f };

    Camera m_shadowCam = Camera::ortho({ 1024.0f, 1024.0f });
    XMFLOAT3 m_shadowDirection{ 0.0f, 0.0f, 0.0f };
    std::unordered_map<Renderable*, RenderBatch> m_renderBatches;

    float t = 0.0f;
};

MainLoop::MainLoop(SDL_Window* window, const std::filesystem::path& scenePath) :
    m_window(window)
{
    m_renderer = createRenderer(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForD3D(window);

    m_renderer->initImgui();

    m_models = loadModels(m_renderer.get());

    if (std::filesystem::exists(scenePath)) {
        std::unordered_map<std::string, const ModelAsset*> m;
        for (const auto& model : m_models) {
            m[model.name] = &model;
        }

        m_scene.load(scenePath);

        m_scene.reg.view<components::Renderable>()
            .each([&](components::Renderable& rc) {
                const auto& model = m[rc.name];
                rc.renderable = model->renderable;
                rc.bounds = model->bounds;
            });
    }

    game = std::make_unique<Game>(m_scene, m_inputs);
    editor = std::make_unique<SceneEditor>(m_scene, m_inputs, m_models);
    m_games[0] = game.get();
    m_games[1] = editor.get();

    m_inputs.key(SDLK_ESCAPE).up([&] { m_running = false; });
    m_inputs.key(SDLK_F1).up([&] { m_gameIdx++; m_gameIdx %= m_games.size(); });
    m_inputs.key(SDLK_HOME).up([&] { m_showDemo = !m_showDemo; });

    m_scene.physicsWorld.addBox(25.0f, 1.0f, 25.0f, 0.0f, 0.0f, +1.0f, 0.0f);

    for (const auto& model : m_models) {
        m_renderBatches[model.renderable].renderable = model.renderable;
    }
}

MainLoop::~MainLoop()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void MainLoop::handleEvents()
{
    auto& io = ImGui::GetIO();
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type) {
        case SDL_QUIT:
            m_running = false;
            break;

        case SDL_KEYUP:
        case SDL_KEYDOWN:
            if (!io.WantCaptureKeyboard) {
                m_inputs.handleEvent(event.key);
            }
            break;

        case SDL_MOUSEMOTION:
            if (!io.WantCaptureMouse) {
                m_inputs.handleEvent(event.motion);
                m_mouse.x = float(event.motion.x) / 1920.0f;
                m_mouse.y = float(event.motion.y) / 1080.0f;
            }
            break;

        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
            if (!io.WantCaptureMouse) {
                auto& ad = Im3d::GetAppData();

                if (event.button.button == SDL_BUTTON_LEFT) {
                    ad.m_keyDown[Im3d::Action_Select] = event.button.state == SDL_PRESSED;
                }

                m_inputs.handleEvent(event.button);
            }
            break;

        default:
            break;
        }
    }
}

void MainLoop::update()
{
    PostProcessParams params;

    auto g = m_games[m_gameIdx];
    float dt = m_gameTime.update();
    t += dt;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplSDL2_NewFrame(m_window);
    ImGui::NewFrame();

    {
        const auto& cam = g->getCamera();
        auto& ad = Im3d::GetAppData();

        ad.m_deltaTime = dt;
        ad.m_viewportSize = Im3d::Vec2(1920.0f, 1080.0f);
        ad.m_worldUp = Im3d::Vec3(0.0f, 1.0f, 0.0f);
        ad.m_projOrtho = false;
        ad.m_projScaleY = tanf(cam.getFOV() / 2.0f) * 2.0f;
        ad.m_snapTranslation = 1.0f;
        ad.m_snapRotation = XMConvertToRadians(10.0f);

        ad.m_viewOrigin = cam.getPosition();
        ad.m_viewDirection = cam.viewToWorld(Vector<View>(0.0f, 0.0f, 1.0f));

        ad.m_cursorRayOrigin = cam.getPosition();
        ad.m_cursorRayDirection = cam.pixelToWorldDirection(int(m_mouse.x * 1920.0f), int(m_mouse.y * 1080.0f));
    }

    Im3d::NewFrame();

    g->update(dt);
    m_scene.physicsWorld.render();

    if (m_showDemo) {
        ImGui::ShowDemoWindow(&m_showDemo);
    }

    if constexpr (false) {
        ImGui::Begin("Post processing");

        ImGui::SliderFloat("Exposure", &params.exposure, 0.0f, 10.0f);
        ImGui::Checkbox("Gamma correction", &params.gammaCorrection);

        ImGui::End();
    }

    {
        m_shadowCam.setRotation(m_scene.directionalLight.x, m_scene.directionalLight.y);
        m_shadowCam.update();

        auto rotation = XMQuaternionRotationRollPitchYaw(m_scene.directionalLight.x, m_scene.directionalLight.y, 0.0f);
        auto direction = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), rotation);
        XMFLOAT3 d;
        XMStoreFloat3(&d, direction);
        m_renderer->setDirectionalLight(d, m_scene.directionalLightColor, m_scene.directionalLightIntensity);

        updateLights();

        m_renderer->setPointLights(m_lights);
    }

    ImGui::Render();
    Im3d::EndFrame();

    updateBatches();

    m_renderer->beginShadowPass(m_shadowCam);
    {
        for (const auto& [_, batch] : m_renderBatches) {
            if (!batch.instances.empty()) {
                m_renderer->drawShadow(batch);
            }
        }
    }
    m_renderer->endShadowPass();

    m_renderer->beginFrame(g->getCamera());
    {
        m_renderer->clear(0.0f, 0.0f, 0.0f);

        for (const auto& [_, batch] : m_renderBatches) {
            if (!batch.instances.empty()) {
                m_renderer->draw(batch);
            }
        }

        g->render(m_renderer.get());

        params.deltaTime = dt;
        m_renderer->postProcess(params);

        m_renderer->drawIm3d(g->getCamera(), ArrayView(Im3d::GetDrawLists(), Im3d::GetDrawListCount()));

        m_renderer->drawImgui();
    }
    m_renderer->endFrame();
}

void MainLoop::updateLights()
{
    m_lights.clear();
    m_scene.reg.view<components::Transform, components::PointLight>()
        .each([&](const components::Transform& tc, const components::PointLight& plc) {
            PointLight l;

            l.Color.x = plc.color.x;
            l.Color.y = plc.color.y;
            l.Color.z = plc.color.z;
            l.Color.w = plc.quadraticAttenuation;

            l.Position.x = tc.position.x;
            l.Position.y = tc.position.y;
            l.Position.z = tc.position.z;
            l.Position.w = plc.linearAttenuation;

            l.Intensity = plc.intensity;
            m_lights.push_back(l);
        });
}

void MainLoop::updateBatches()
{
    for (auto& [_, batch] : m_renderBatches) {
        batch.instances.clear();
    }

    m_scene.reg.view<components::Transform, components::Renderable>()
        .each([&](const components::Transform& t, const components::Renderable& rc) {
            auto wm = t.getMatrix();
            auto& instance = m_renderBatches[rc.renderable].instances.emplace_back();
            instance.World = XMMatrixTranspose(wm);
            instance.WorldInvTranspose = XMMatrixInverse(nullptr, wm);
        });
}

int main(int argc, char* argv[])
{
    auto args = getArgs(argc, argv);

    if (args.size() > 2 && args[1] == "convert") {
        for (int i = 2; i < argc; i++) {
            convertAssets(args[i], "./content");
        }

        return 0;
    }

    if (auto ret = SDL_Init(SDL_INIT_VIDEO); ret < 0) {
        reportError("SDL_Init returned {}", ret);
        return 0;
    }

    auto window = SDL_CreateWindow("ebin", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_SHOWN);
    if (!window) {
        reportError("SDL_CreateWindow returned nullptr");
        SDL_Quit();
        return 0;
    }

    bool running = true;
    std::filesystem::path scenePath;

    while (running) {
        try {
            MainLoop mainLoop(window, scenePath);

            do {
                mainLoop.handleEvents();
                mainLoop.update();
                running = mainLoop.isRunning();
            } while (mainLoop.isRunning());
        } catch (const LoadSceneException& lse) {
            scenePath = lse.path;
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

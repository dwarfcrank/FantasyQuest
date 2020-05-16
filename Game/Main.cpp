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
#include "im3d.h"

#include <SDL2/SDL.h>
#include <fmt/format.h>
#include <type_traits>
#include <DirectXMath.h>
#include <chrono>
#include <entt/entt.hpp>
#include <unordered_set>
#include <random>

#include <bullet/btBulletDynamicsCommon.h>

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
            models.emplace_back(mesh.getName(), renderable, mesh.getBounds());
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

struct PhysicsWorld
{
    PhysicsWorld()
    {
        collisionConfiguration.reset(new btDefaultCollisionConfiguration());
        dispatcher.reset(new btCollisionDispatcher(collisionConfiguration.get()));
        overlappingPairCache.reset(new btDbvtBroadphase());
        solver.reset(new btSequentialImpulseConstraintSolver());
        dynamicsWorld.reset(new btDiscreteDynamicsWorld(
            dispatcher.get(), overlappingPairCache.get(), solver.get(), collisionConfiguration.get()));

        dynamicsWorld->setGravity(btVector3(0.0f, -10.0f, 0.0f));
    }

    void addBox(float hw, float hh, float hd, float mass, float x, float y, float z)
    {
        auto shape = collisionShapes.emplace_back(new btBoxShape(btVector3(hw, hh, hd))).get();

        btTransform t;
        t.setIdentity();
        t.setOrigin(btVector3(x, y, z));

        auto motionState = new btDefaultMotionState(t);
        btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, shape);
        auto body = new btRigidBody(info);

        dynamicsWorld->addRigidBody(body);
    }

    void addSphere(float radius, float mass, float x, float y, float z)
    {
        auto shape = collisionShapes.emplace_back(new btSphereShape(radius)).get();

        btTransform t;
        t.setIdentity();
        t.setOrigin(btVector3(x, y, z));

        btVector3 localInertia(0.0f, 0.0f, 0.0f);
        shape->calculateLocalInertia(mass, localInertia);

        auto motionState = new btDefaultMotionState(t);
        btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, shape, localInertia);
        auto body = new btRigidBody(info);

        dynamicsWorld->addRigidBody(body);
    }

    void update(float dt)
    {
        time += dt;

        if (time < TIMESTEP) {
            return;
        }

        time -= TIMESTEP;

        dynamicsWorld->stepSimulation(TIMESTEP, 10);
    }

    void render()
    {
        Transform tt;
        btTransform ft;

        auto n = dynamicsWorld->getNumCollisionObjects();
        const auto& collisionObjects = dynamicsWorld->getCollisionObjectArray();

        for (int i = 0; i < n; i++) {
            auto obj = collisionObjects[i];

            if (auto body = btRigidBody::upcast(obj)) {
                if (auto motionState = body->getMotionState()) {
                    motionState->getWorldTransform(ft);
                }
            } else {
                ft = obj->getWorldTransform();
            }

            const auto& origin = ft.getOrigin();
            XMFLOAT3 position(origin.x(), origin.y(), origin.z());

            XMFLOAT3 rotation;
            ft.getRotation().getEulerZYX(rotation.z, rotation.y, rotation.x);
            
            tt.Position = XMLoadFloat3(&position);
            tt.Rotation = XMLoadFloat3(&rotation);

            XMFLOAT4X4 tt2;
            XMStoreFloat4x4(&tt2, tt.getMatrix());
            Im3d::Mat4 tm;
            std::memcpy(&tm, &tt2, sizeof(tm));

            const auto* shape = obj->getCollisionShape();

            Im3d::PushMatrix(tm);
            Im3d::PushSize(2.5f);
            Im3d::PushColor(Im3d::Color_Cyan);
            if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE) {
                const auto* box = static_cast<const btBoxShape*>(shape);
                const auto halfExtents = box->getHalfExtentsWithoutMargin();

                Im3d::Vec3 h(halfExtents.x(), halfExtents.y(), halfExtents.z());
                Im3d::DrawAlignedBox({ -h.x, -h.y, -h.z }, h);
            } else if (shape->getShapeType() == SPHERE_SHAPE_PROXYTYPE) {
                const auto* sphere = static_cast<const btSphereShape*>(shape);
                Im3d::DrawSphere(Im3d::Vec3(0.0f), sphere->getRadius());
            }
            Im3d::PopColor();
            Im3d::PopSize();
            Im3d::PopMatrix();
        }
    }

    std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btBroadphaseInterface> overlappingPairCache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
    std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld;

    std::vector<std::unique_ptr<btCollisionShape>> collisionShapes;

    float time = 0.0f;
    static constexpr auto TICKS_PER_SECOND = 60;
    static constexpr auto TIMESTEP = 1.0f / float(TICKS_PER_SECOND);
};

std::vector<std::string_view> getArgs(int argc, char* argv[])
{
    std::vector<std::string_view> args;

    for (int i = 0; i < argc; i++) {
        args.emplace_back(argv[i]);
    }

    return args;
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

    {
        auto r = createRenderer(window);

        Scene scene;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        ImGui::StyleColorsDark();
        ImGui_ImplSDL2_InitForD3D(window);
        r->initImgui();

        InputMap inputs;

        bool running = true;
        inputs.key(SDLK_ESCAPE).up([&] { running = false; });

        auto models = loadModels(r.get());

        if (std::filesystem::exists("scene.json")) {
            std::unordered_map<std::string, const ModelAsset*> m;
            for (const auto& model : models) {
                m[model.name] = &model;
            }

            scene.load("scene.json");

            scene.reg.view<components::Renderable>()
                .each([&](components::Renderable& rc) {
                    const auto& model = m[rc.name];
                    rc.renderable = model->renderable;
                    rc.bounds = model->bounds;
                });
        }

        Game game(scene, inputs);
        SceneEditor editor(scene, inputs, models);

        std::array<GameBase*, 2> games{ &game, &editor };
        size_t gameIdx = 1;
        inputs.key(SDLK_F1).up([&] { gameIdx++; gameIdx %= games.size(); });

        GameTime gt;

        bool showDemo = false;
        inputs.key(SDLK_HOME).up([&] { showDemo = !showDemo; });

        XMFLOAT2 mouse(0.0f, 0.0f);

        auto handleEvents = [&io, &inputs, &running, &mouse] {
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
                        mouse.x = float(event.motion.x) / 1920.0f;
                        mouse.y = float(event.motion.y) / 1080.0f;
                    }
                    break;

                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEBUTTONDOWN:
                    if (!io.WantCaptureMouse) {
                        auto& ad = Im3d::GetAppData();

                        if (event.button.button == SDL_BUTTON_LEFT) {
                            ad.m_keyDown[Im3d::Action_Select] = event.button.state == SDL_PRESSED;
                        }
                    }
                    break;

                default:
                    break;
                }
            }
        };

        PhysicsWorld pw;
        pw.addBox(20.0f, 1.0f, 20.0f, 0.0f, 0.0f, 0.0f, 0.0f);

        std::mt19937 gen{ std::random_device{}() };
        std::uniform_real_distribution<float> dist{ -10.0f, 10.0f };
        
        float radius = 1.0f;
        inputs.key(SDLK_SPACE).up([&] () mutable {
            pw.addSphere(radius, radius * 2.5f, dist(gen), 15.0f, dist(gen));
            radius += 0.1f;
        });

        Camera shadowCam = Camera::ortho();

        XMFLOAT3 shadowDir{ 0.0f, 0.0f, 0.0f };

        std::unordered_map<Renderable*, RenderBatch> batches;

        for (const auto& model : models) {
            batches[model.renderable].renderable = model.renderable;
        }

        auto updateBatches = [&] {
            for (auto& [_, batch] : batches) {
                batch.instances.clear();
            }

            scene.reg.view<components::Transform, components::Renderable>()
                .each([&](const components::Transform& t, const components::Renderable& rc) {
					auto wm = t.getMatrix();
					auto& instance = batches[rc.renderable].instances.emplace_back();
					instance.World = XMMatrixTranspose(wm);
					instance.WorldInvTranspose = XMMatrixInverse(nullptr, wm);
				});
        };

        std::vector<PointLight> lights;

        auto updateLights = [&] {
            lights.clear();
            scene.reg.view<components::Transform, components::PointLight>()
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
                    lights.push_back(l);
                });
        };

        PostProcessParams params;

        float t = 0.0f;

        while (running) {
            auto g = games[gameIdx];
            float dt = gt.update();
            t += dt;

            handleEvents();

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplSDL2_NewFrame(window);
            ImGui::NewFrame();

            {
                auto& ad = Im3d::GetAppData();
                ad.m_deltaTime = dt;
                ad.m_viewportSize = Im3d::Vec2(1920.0f, 1080.0f);
                ad.m_worldUp = Im3d::Vec3(0.0f, 1.0f, 0.0f);
                ad.m_projOrtho = false;
                ad.m_projScaleY = 2.0f;

                {
                    const auto ivm = g->getCamera().getInverseViewMatrix();
                    XMFLOAT4X4 proj;
                    XMStoreFloat4x4(&proj, g->getCamera().getProjectionMatrix().mat);

                    XMFLOAT3 c;
                    float cursorX = (mouse.x * 2.0f - 1.0f) / proj._11;
                    float cursorY = -(mouse.y * 2.0f - 1.0f) / proj._22;
                    Vector<View> cursor(cursorX, cursorY, 1.0f, 0.0f);
                    cursor.vec = XMVector3Normalize(cursor.vec);
                    XMStoreFloat3(&c, cursor.vec);

                    ad.m_viewOrigin = g->getCamera().getPosition();
                    ad.m_viewDirection = Vector<View>(0.0f, 0.0f, 1.0f) * ivm;
                    ad.m_cursorRayOrigin = ad.m_viewOrigin;
                    ad.m_cursorRayDirection = cursor * ivm;
                }
            }

            Im3d::NewFrame();

            pw.update(dt);
            g->update(dt);
            pw.render();

            if (showDemo) {
                ImGui::ShowDemoWindow(&showDemo);
            }

            if constexpr (false) {
                ImGui::Begin("Post processing");

                ImGui::SliderFloat("Exposure", &params.exposure, 0.0f, 10.0f);
                ImGui::Checkbox("Gamma correction", &params.gammaCorrection);

                ImGui::End();
            }

            {
                shadowCam.setRotation(scene.directionalLight.x, scene.directionalLight.y);

                auto rotation = XMQuaternionRotationRollPitchYaw(scene.directionalLight.x, scene.directionalLight.y, 0.0f);
                auto direction = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), rotation);
                XMFLOAT3 d;
                XMStoreFloat3(&d, direction);
                r->setDirectionalLight(d, scene.directionalLightColor, scene.directionalLightIntensity);

                updateLights();

                r->setPointLights(lights);
            }

            ImGui::Render();
            Im3d::EndFrame();

            updateBatches();

            r->beginShadowPass(shadowCam);
            {
                for (const auto& [_, batch] : batches) {
                    if (!batch.instances.empty()) {
                        r->drawShadow(batch);
                    }
                }
            }
            r->endShadowPass();

            r->beginFrame(g->getCamera());
            {
                r->clear(0.0f, 0.0f, 0.0f);

                for (const auto& [_, batch] : batches) {
                    if (!batch.instances.empty()) {
                        r->draw(batch);
                    }
                }

                g->render(r.get());

                params.deltaTime = dt;
                r->postProcess(params);

                r->drawIm3d(g->getCamera(), ArrayView(Im3d::GetDrawLists(), Im3d::GetDrawListCount()));

                r->drawImgui();
            }
            r->endFrame();
        }

        scene.save("scene.json");

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

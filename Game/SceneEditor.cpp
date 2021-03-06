#include "pch.h"

#include "SceneEditor.h"
#include "Scene.h"
#include "InputMap.h"
#include "Math.h"
#include "PhysicsWorld.h"
#include "Mesh.h"

#include "Components/BasicProperties.h"
#include "Components/Transform.h"
#include "Components/Renderable.h"
#include "Components/PointLight.h"

#include <im3d.h>
#include <im3d_math.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <algorithm>
#include <unordered_map>

using namespace math;

static std::string_view getPrefix(std::string_view name)
{
    if (auto i = name.find_first_of('_'); i != std::string::npos && i > 0) {
        return name.substr(0, i);
    }

    //return std::string_view();
    return name;
}

// TODO: make this part of Camera itself
static WorldVector screenToWorldDirection(const Camera& camera, int x, int y, const XMFLOAT2& dimensions)
{
    XMFLOAT2 screenPos(
        (float(x) / dimensions.x) * 2.0f - 1.0f,
        -(float(y) / dimensions.y) * 2.0f + 1.0f
    );

    auto pos = camera.getPosition();
    auto dir = camera.viewToWorld({ screenPos.x, screenPos.y, 1.0f, 1.0f }) - pos;

    return dir.normalized();
}

SceneEditor::SceneEditor(Scene& scene, InputMap& inputs, const std::vector<ModelAsset>& models) :
    GameBase(inputs), m_scene(scene), m_models(models), m_nextId(int(scene.reg.size()))
{
    auto doBind = [this](SDL_Keycode k, float& target, float value) {
        m_inputs.key(k)
            .down([&target, value] { target = value; })
            .up([&target] { target = 0.0f; });
    };

    doBind(SDLK_q, angle, -turnSpeed);
    doBind(SDLK_e, angle, turnSpeed);

    doBind(SDLK_d, velocity.x, moveSpeed);
    doBind(SDLK_a, velocity.x, -moveSpeed);
    doBind(SDLK_w, velocity.z, moveSpeed);
    doBind(SDLK_s, velocity.z, -moveSpeed);

    doBind(SDLK_r, velocity.y, moveSpeed);
    doBind(SDLK_f, velocity.y, -moveSpeed);

    m_inputs.key(SDLK_1).up([] { Im3d::GetContext().m_gizmoMode = Im3d::GizmoMode_Translation; });
    m_inputs.key(SDLK_2).up([] { Im3d::GetContext().m_gizmoMode = Im3d::GizmoMode_Rotation; });
    m_inputs.key(SDLK_3).up([] { Im3d::GetContext().m_gizmoMode = Im3d::GizmoMode_Scale; });

    m_inputs.key(SDLK_c).up([&] { m_currentEntity = createEntity(); });

    m_inputs.mouseButton(SDL_BUTTON_LEFT).doubleClick([&](int x, int y) {
        auto from = m_camera.getPosition();
        auto to = from + (100.0f * m_camera.pixelToWorldDirection(x, y));

        if (auto hit = m_scene.physicsWorld.raycast(from, to)) {
            m_currentEntity = hit.entity;
        } else {
            m_currentEntity = entt::null;
        }
    });

    m_inputs.onMouseMove([this](const SDL_MouseMotionEvent& event) {
        if ((event.state & SDL_BUTTON_RMASK) || (moveCamera)) {
            auto x = static_cast<float>(event.xrel) / 450.0f;
            auto y = static_cast<float>(event.yrel) / 450.0f;
            m_camera.rotate(y, x);
        }
    });

    std::sort(m_models.begin(), m_models.end(),
        [](const auto& a, const auto& b) {
            return a.name < b.name;
        });

    m_camera.setPosition({ 0.0f, 5.0f, -5.0f });

    m_componentEditors[entt::type_info<components::Renderable>::id()] = &SceneEditor::renderableComponentEditor;
    m_componentEditors[entt::type_info<components::PointLight>::id()] = &SceneEditor::pointLightComponentEditor;
    m_componentEditors[entt::type_info<components::Collision>::id()] = &SceneEditor::collisionComponentEditor;
    m_componentEditors[entt::type_info<components::Physics>::id()] = &SceneEditor::physicsComponentEditor;
}

static_assert(sizeof(XMFLOAT4X4A) == sizeof(Im3d::Mat4));

bool SceneEditor::update(float dt)
{
    moveCamera = (SDL_GetModState() & KMOD_LCTRL) != 0;

    drawGrid();

    XMFLOAT3 v(velocity.x * dt, velocity.y * dt, velocity.z * dt);
    float changed = (v.x != 0.0f) || (v.y != 0.0f) || (v.z != 0.0f) || (angle != 0.0f);
    float a = angle * dt;

    if (entitySelected()) {
        const auto& t = m_scene.reg.get<components::Transform>(m_currentEntity);

        Im3d::Mat4 transform = t.getMatrix();

        Im3d::PushLayerId("currentEntity");
        if (Im3d::Gizmo("gizmo", transform) && !m_physicsEnabled) {
            auto pos = transform.getTranslation();
            auto rotationQuat = XMQuaternionRotationMatrix(transform);

            m_scene.reg.patch<components::Transform>(m_currentEntity, [&](components::Transform& tc) {
                tc.position = pos;
                tc.rotationQuat = rotationQuat;
                tc.scale = transform.getScale();
            });

            btQuaternion rot;
            rot.set128(rotationQuat);

            btTransform pt(rot, btVector3(pos.x, pos.y, pos.z));

            // TODO: use patch here
            if (auto pc = m_scene.reg.try_get<components::Physics>(m_currentEntity)) {
                pc->collisionObject->setWorldTransform(pt);
            } else if (auto cc = m_scene.reg.try_get<components::Collision>(m_currentEntity)) {
                cc->collisionObject->setWorldTransform(pt);
            }
        }
        Im3d::PopLayerId();

        drawEntityBounds(m_currentEntity);
    }
    
    if (moveCamera) {
        m_camera.move(ViewVector{ v.x, v.y, v.z });
        m_camera.rotate(0.0f, a);
    }

    m_camera.update();

    sceneWindow();
    entityPropertiesWindow();
    modelList();
    mainMenu();
    
    if (m_physicsEnabled) {
        m_scene.physicsWorld.update(dt);
    } else {
        m_scene.physicsWorld.editorUpdate();
    }

    return true;
}

void SceneEditor::render(IRenderer* r)
{
}

const Camera& SceneEditor::getCamera() const
{
    return m_camera;
}

bool SceneEditor::entitySelected() const
{
    return m_scene.reg.valid(m_currentEntity);
}

void SceneEditor::modelList()
{
    struct ModelPrefixComparison
    {
        bool operator()(const ModelAsset& m, std::string_view p) const { return getPrefix(m.name) < p; }
        bool operator()(std::string_view p, const ModelAsset& m) const { return p < getPrefix(m.name); }
    };

    if (ImGui::Begin("Models")) {
        int i = 0;
        int selection = -1;

        auto modelListEntry = [&](const ModelAsset& model) {
            ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf 
                | ImGuiTreeNodeFlags_NoTreePushOnOpen
                | ImGuiTreeNodeFlags_OpenOnDoubleClick
                | ImGuiTreeNodeFlags_SpanAvailWidth;

            if (i == m_currentModelIdx) {
                nodeFlags |= ImGuiTreeNodeFlags_Selected;
            }

            ImGui::TreeNodeEx(model.name.c_str(), nodeFlags);

            if (i == m_currentModelIdx) {
                if (ImGui::IsItemHovered()) {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        m_currentEntity = createEntity();
                    }
                }
            } else if (ImGui::IsItemClicked()) {
                if (i == m_currentModelIdx) {
                    m_currentModelIdx = -1;
                    selection = -1;
                } else {
                    selection = i;
                }
            }

            i++;
        };

        auto it = m_models.cbegin();
        while (it != m_models.cend()) {
            auto prefix = getPrefix(it->name);
            auto [begin, end] = std::equal_range(it, m_models.cend(), prefix, ModelPrefixComparison{});

            if (begin != it) {
                modelListEntry(*it);
                ++it;
                continue;
            }

            auto dist = std::distance(begin, end);

            if (dist == 1) {
                modelListEntry(*begin);
            } else {
                if (std::string ps(prefix); ImGui::TreeNode(ps.c_str())) {
                    std::for_each(begin, end, modelListEntry);
                    ImGui::TreePop();
                } else {
                    // tree node is closed so the loop won't run, we have to advance i for
                    // every model that would've been under it
                    i += int(dist);
                }
            }

            it = end;
        }

        if (selection != -1) {
            m_currentModelIdx = selection;
        }
    }

    ImGui::End();
}

void SceneEditor::entityList()
{
    entt::entity toDestroy = entt::null;

    if (ImGui::BeginChild("##entities", ImVec2(-1.0f, -1.0f))) {
        ImGui::Columns(2, nullptr, false);
        auto w = ImGui::GetWindowContentRegionWidth();

        m_scene.reg.view<components::Misc>()
            .each([&](entt::entity entity, const components::Misc& m) {
                ImGui::SetColumnWidth(-1, w - 25.0f);
				if (ImGui::Selectable(m.name.c_str(), entity == m_currentEntity)) {
                    if (m_currentEntity == entity) {
                        m_currentEntity = entt::null;
                    } else {
                        m_currentEntity = entity;
                    }
				}

                ImGui::NextColumn();

                ImGui::PushID(m.name.c_str());
                if (ImGui::SmallButton("x")) {
                    toDestroy = entity;
                }
                ImGui::PopID();

                ImGui::NextColumn();
			});

        ImGui::Columns(1);
    }

    ImGui::EndChild();

    if (m_scene.reg.valid(toDestroy)) {
        m_scene.reg.destroy(toDestroy);
    }
}

void SceneEditor::entityPropertiesWindow()
{
    if (ImGui::Begin("Entity")) {
        if (!entitySelected()) {
            ImGui::Text("No entity selected");
            ImGui::End();
            return;
        }

        {
            bool changed = false;

            auto& m = m_scene.reg.get<components::Misc>(m_currentEntity);
            if (ImGui::InputText("Name", &m.name)) {
                // TODO: one day I will need to handle this...
            }

            ImGui::Separator();

            auto& t = m_scene.reg.get<components::Transform>(m_currentEntity);

            changed |= ImGui::InputFloat3("Position", &t.position.x);

            changed |= ImGui::SliderAngle("X", &t.rotation.x, -180.0f, 180.0f);
            changed |= ImGui::SliderAngle("Y", &t.rotation.y, -180.0f, 180.0f);
            changed |= ImGui::SliderAngle("Z", &t.rotation.z, -180.0f, 180.0f);
            changed |= ImGui::InputFloat3("Scale", &t.scale.x);

            if (changed) {
                auto rotationQuat = XMQuaternionRotationRollPitchYaw(t.rotation.x, t.rotation.y, t.rotation.z);

                m_scene.reg.patch<components::Transform>(m_currentEntity, [&](components::Transform& tc) {
                    tc.rotationQuat = rotationQuat;
                });

                btQuaternion rot;
                rot.set128(rotationQuat);

                btTransform pt(rot, btVector3(t.position.x, t.position.y, t.position.z));

                // TODO: physics and collision components should have a common interface
                if (auto pc = m_scene.reg.try_get<components::Physics>(m_currentEntity)) {
                    pc->collisionObject->setWorldTransform(pt);
                } else if (auto cc = m_scene.reg.try_get<components::Collision>(m_currentEntity)) {
                    cc->collisionObject->setWorldTransform(pt);
                }
            }
        }

        m_scene.reg.visit(m_currentEntity, [&](const entt::id_type cid) {
            if (auto it = m_componentEditors.find(cid); it != m_componentEditors.end()) {
                ImGui::Separator();
                (this->*(it->second))(m_currentEntity);
            }
        });
    }

    ImGui::End();
}

void SceneEditor::renderableComponentEditor(entt::entity e)
{
    auto rc = m_scene.reg.try_get<components::Renderable>(e);
    bool render = (rc != nullptr);

    if (ImGui::Checkbox("Render", &render)) {
        if (!render) {
            m_scene.reg.remove_if_exists<components::Renderable>(e);
            rc = nullptr;
        } else if (render && !rc) {
            const auto& model = m_models.front();
            auto& rc = m_scene.reg.emplace<components::Renderable>(e,
                model.name, model.renderable, model.bounds);
        }
    }

    if (rc) {
        int selected = 0;

        for (size_t i = 0; i < m_models.size(); i++) {
            if (rc->name == m_models[i].name) {
                selected = int(i);
                break;
            }
        }

        int newSelection = selected;

        auto getter = [](void* data, int idx, const char** out) {
            const auto& items = *reinterpret_cast<const decltype(m_models)*>(data);

            *out = items[idx].name.c_str();

            return true;
        };

        ImGui::Combo("Mesh", &newSelection, getter, &m_models, int(m_models.size()));

        if (ImGui::Button("Pick from model list")) {
            newSelection = int(m_currentModelIdx);
        }

        if (newSelection != selected) {
            m_scene.reg.patch<components::Renderable>(e,
                [&](components::Renderable& r) {
                    const auto& model = m_models[newSelection];
                    r.name = model.name;
                    r.renderable = model.renderable;
                    r.bounds = model.bounds;
                });
        }
    }
}

void SceneEditor::physicsComponentEditor(entt::entity e)
{
    bool hasPhysics = m_scene.reg.has<components::Physics>(e);
    bool created = false;

    if (ImGui::Checkbox("Physics", &hasPhysics)) {
        if (!hasPhysics) {
            m_scene.reg.remove_if_exists<components::Physics>(e);
        } else {
            m_scene.reg.emplace<components::Physics>(e);
            created = true;
        }
    }

    if (auto pc = m_scene.reg.try_get<components::Physics>(e); hasPhysics && pc) {
        if (const auto* rc = m_scene.reg.try_get<components::Renderable>(e); rc && created) {
            pc->collisionShape = getCollisionMesh(rc->name);
            pc->collisionObject->setCollisionShape(pc->collisionShape);
        }

        bool changed = false;

        // TODO: move this to the physics component or something
        if (auto rb = btRigidBody::upcast(pc->collisionObject.get())) {
            if (ImGui::InputFloat("Mass", &pc->mass)) {
                btVector3 inertia(0.0f, 0.0f, 0.0f);
                if (pc->mass != 0.0f) {
                    pc->collisionShape->calculateLocalInertia(pc->mass, inertia);
                }

                rb->setMassProps(pc->mass, inertia);
            }
        }

        int idx = pc->collisionShape->getUserIndex();
        auto previewText = "(null)";
        if (idx >= 0 && idx < int(m_models.size())) {
            previewText = m_models[idx].name.c_str();
        }

        if (ImGui::BeginCombo("Collision mesh", previewText)) {
            for (int i = 0; const auto& model : m_models) {
                if (ImGui::Selectable(model.name.c_str(), i == idx)) {
                    pc->collisionShape = getCollisionMesh(model.name);
                    pc->collisionObject->setCollisionShape(pc->collisionShape);
                }

                i++;
            }

            ImGui::EndCombo();
        }
    }
}

void SceneEditor::collisionComponentEditor(entt::entity e)
{
    bool hasCollision = m_scene.reg.has<components::Collision>(e);
    bool created = false;

    if (ImGui::Checkbox("Collision", &hasCollision)) {
        if (!hasCollision) {
            m_scene.reg.remove_if_exists<components::Collision>(e);
        } else {
            m_scene.reg.emplace<components::Collision>(e);
            created = true;
        }
    }

    if (auto cc = m_scene.reg.try_get<components::Collision>(e); hasCollision && cc) {
        if (const auto* rc = m_scene.reg.try_get<components::Renderable>(e); rc && created) {
            cc->collisionShape = getCollisionMesh(rc->name);
            cc->collisionObject->setCollisionShape(cc->collisionShape);
        }

        bool changed = false;

        int idx = cc->collisionShape->getUserIndex();
        auto previewText = "(null)";
        if (idx >= 0 && idx < int(m_models.size())) {
            previewText = m_models[idx].name.c_str();
        }

        if (ImGui::BeginCombo("Collision mesh", previewText)) {
            for (int i = 0; const auto& model : m_models) {
                if (ImGui::Selectable(model.name.c_str(), i == idx)) {
                    cc->collisionShape = getCollisionMesh(model.name);
                    cc->collisionObject->setCollisionShape(cc->collisionShape);
                }

                i++;
            }

            ImGui::EndCombo();
        }
    }
}

void SceneEditor::pointLightComponentEditor(entt::entity e)
{
    auto plc = m_scene.reg.try_get<components::PointLight>(e);
    bool hasLight = (plc != nullptr);

    if (ImGui::Checkbox("Point light", &hasLight)) {
        if (!hasLight) {
            m_scene.reg.remove_if_exists<components::PointLight>(e);
            plc = nullptr;
        } else if (hasLight && !plc) {
            m_scene.reg.emplace<components::PointLight>(e);
        }
    }

    if (plc) {
        bool changed = false;

        changed |= ImGui::ColorEdit3("Color", &plc->color.x);
        changed |= ImGui::SliderFloat("Linear", &plc->linearAttenuation, 0.0f, 5.0f, "%.5f");
        changed |= ImGui::SliderFloat("Quadratic", &plc->quadraticAttenuation, 0.0f, 5.0f, "%.5f");
        changed |= ImGui::SliderFloat("Intensity", &plc->intensity, 0.0f, 100.0f, "%.5f");
        //changed |= ImGui::SliderFloat("Radius", &plc->radius, 0.0f, 100.0f, "%.5f");
        changed |= ImGui::DragFloat("Radius", &plc->radius);

        if (changed) {
            if (std::fabs(plc->radius) <= FLT_EPSILON) {
                plc->radius = 1.0f;
            }
        }
    }
}

btCollisionShape* SceneEditor::getCollisionMesh(const std::string& name)
{
    if (auto collisionMesh = m_scene.physicsWorld.getCollisionMesh(name)) {
        return collisionMesh;
    }

    auto it = std::find_if(m_models.cbegin(), m_models.cend(), [&](const ModelAsset& m) {
        return m.name == name;
    });

    if (it != m_models.cend()) {
        int idx = int(std::distance(m_models.cbegin(), it));
        Mesh mesh;
        mesh.load(it->filename);

        auto collisionShape = m_scene.physicsWorld.createCollisionMesh(mesh.getName(), mesh);
        collisionShape->setUserIndex(idx);

        return collisionShape;
    }

    return nullptr;
}

void SceneEditor::sceneWindow()
{
    if (ImGui::Begin("Scene")) {
        ImGui::InputText("Name", &m_scene.name);
        ImGui::Separator();
        ImGui::Checkbox("Simulate physics", &m_physicsEnabled);

        if (ImGui::Checkbox("Draw physics models", &m_drawPhysics)) {
            m_scene.physicsWorld.setDebugDrawMode(m_drawPhysics ? btIDebugDraw::DBG_DrawWireframe : 0);
        }

        ImGui::Separator();

        if (ImGui::Button("New entity")) {
            m_currentEntity = createEntity();
        }

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Directional light", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& dir = m_scene.directionalLight;

            bool changed = false;

            changed |= ImGui::SliderAngle("X", &dir.x, -180.0f, 180.0f);
            changed |= ImGui::SliderAngle("Y", &dir.y, -180.0f, 180.0f);
            changed |= ImGui::ColorEdit3("Color", &m_scene.directionalLightColor.x);
            changed |= ImGui::SliderFloat("Intensity", &m_scene.directionalLightIntensity, 0.0f, 10.0f, "%.5f");
        }

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
            entityList();
        }

        ImGui::Separator();
    }

    ImGui::End();
}

entt::entity SceneEditor::createEntity()
{
    auto e = m_scene.reg.create();

    components::Transform t;

    if (entitySelected()) {
        t = m_scene.reg.get<components::Transform>(m_currentEntity);
    }

    const auto& model = m_models[m_currentModelIdx];

    m_scene.reg.emplace<components::Misc>(e, fmt::format("{}:{}", model.name, m_nextId++));
    m_scene.reg.emplace<components::Transform>(e, t);
    m_scene.reg.emplace<components::Renderable>(e, model.name, model.renderable, model.bounds);
    //m_scene.reg.emplace<components::Physics>(e);

    return e;
}

static void listScenes()
{
    if (!std::filesystem::exists("./scenes")) {
        return;
    }

    std::filesystem::directory_iterator end{};

    for (auto it = std::filesystem::directory_iterator("./scenes"); it != end; ++it) {
        if (!it->is_regular_file()) {
            continue;
        }

        if (auto p = it->path(); p.extension() == ".json") {
            const auto s = p.filename().generic_string();

            if (ImGui::MenuItem(s.c_str())) {
                throw LoadSceneException{ .path = p };
            }
        }
    }
}

void SceneEditor::mainMenu()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::BeginMenu("Open...")) {
                listScenes();
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Save", "CTRL+S")) {
                // TODO: really should keep the scene filename around somewhere
                const std::filesystem::path root("./scenes");
                std::filesystem::create_directories(root);
                m_scene.save((root / m_scene.name).replace_extension(".json"));
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void SceneEditor::drawGrid()
{
    Im3d::PushDrawState();

    float size = 1.5f;
    Im3d::SetSize(size);
    Im3d::DrawLine({ -100.0f, 0.0f, 0.0f }, { 100.0f, 0.0f, 0.0f }, size, Im3d::Color_White);
    Im3d::DrawLine({ 0.0f, 0.0f, -100.0f }, { 0.0f, 0.0f, 100.0f }, size, Im3d::Color_White);

    constexpr u32 gridColor2 = 0x808080ff;

    for (auto i = 1; i <= 20; i++) {
        Im3d::DrawLine({ float(i) *  5.0f, 0.0f, 100.0f }, { float(i) *  5.0f, 0.0f, -100.0f }, size, gridColor2);
        Im3d::DrawLine({ float(i) * -5.0f, 0.0f, 100.0f }, { float(i) * -5.0f, 0.0f, -100.0f }, size, gridColor2);
        Im3d::DrawLine({ 100.0f, 0.0f, float(i) *  5.0f }, { -100.0f, 0.0f, float(i) *  5.0f }, size, gridColor2);
        Im3d::DrawLine({ 100.0f, 0.0f, float(i) * -5.0f }, { -100.0f, 0.0f, float(i) * -5.0f }, size, gridColor2);
    }

    Im3d::PopDrawState();
}

void SceneEditor::drawEntityBounds(entt::entity e)
{
    if (!m_scene.reg.valid(e)) {
        return;
    }

    const auto t = std::as_const(m_scene.reg).try_get<components::Transform>(e);

    if (!t) {
        return;
    }

    Im3d::PushDrawState();
    Im3d::PushMatrix();
    Im3d::SetMatrix(t->getMatrix());
    Im3d::SetSize(3.0f);

    if (const auto rc = std::as_const(m_scene.reg).try_get<components::Renderable>(e)) {
        XMFLOAT3 bMin, bMax;
        XMStoreFloat3(&bMin, rc->bounds.min.vec);
        XMStoreFloat3(&bMax, rc->bounds.max.vec);

        Im3d::DrawAlignedBox(Im3d::Vec3(bMin.x, bMin.y, bMin.z), Im3d::Vec3(bMax.x, bMax.y, bMax.z));
    } else if (const auto plc = std::as_const(m_scene.reg).try_get<components::PointLight>(e)) {
        Im3d::DrawSphere(Im3d::Vec3(0.0f), std::max(plc->radius, 1.0f));
    }

    Im3d::PopMatrix();
    Im3d::PopDrawState();
}

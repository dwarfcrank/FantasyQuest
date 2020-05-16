#include "pch.h"

#include "SceneEditor.h"
#include "Scene.h"
#include "imgui.h"
#include "InputMap.h"
#include "imgui_stdlib.h"
#include "im3d.h"
#include "im3d_math.h"
#include "Math.h"

using namespace math;

SceneEditor::SceneEditor(Scene& scene, InputMap& inputs, const std::vector<ModelAsset>& models) :
    GameBase(inputs), m_scene(scene), m_models(models)
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

    m_camera.setPosition(0.0f, 5.0f, -5.0f);
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
        auto& t = m_scene.reg.get<components::Transform>(m_currentEntity);

        Im3d::Mat4 transform(1.0f);
        {
            XMFLOAT4X4A t2;
            XMStoreFloat4x4A(&t2, t.getMatrix());
            std::memcpy(&transform, &t2, sizeof(transform));
        }

        Im3d::PushLayerId("gizmo");
        if (Im3d::Gizmo("currentEntity", transform)) {
            t.position = transform.getTranslation();
            // TODO: this doesn't work
            //t.rotation = Im3d::ToEulerXYZ(transform.getRotation());
            t.scale = transform.getScale();
        }
        Im3d::PopLayerId();

        drawEntityBounds(m_currentEntity);
    }
    
    if (moveCamera) {
        m_camera.move(v.x, v.y, v.z);
        m_camera.rotate(0.0f, a);
    }

    sceneWindow();
    entityPropertiesWindow();
    modelList();
    
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
    if (ImGui::Begin("Models")) {
        for (size_t i = 0; i < m_models.size(); i++) {
            if (ImGui::Selectable(m_models[i].name.c_str(), i == m_currentModelIdx)) {
                m_currentModelIdx = i;
            }
        }
    }

    ImGui::End();
}

void SceneEditor::entityList()
{
    entt::entity e = entt::null;

    if (ImGui::BeginChild("##entities", ImVec2(-1.0f, -1.0f))) {
        m_scene.reg.view<components::Misc>()
            .each([&](entt::entity entity, const components::Misc& m) {
				if (ImGui::Selectable(m.name.c_str(), entity == m_currentEntity)) {
                    if (m_currentEntity == entity) {
                        m_currentEntity = entt::null;
                    } else {
                        m_currentEntity = entity;
                    }
				}
			});
    }

    ImGui::EndChild();
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
                // hmmm
            }
        }

        ImGui::Separator();

        {
            auto rc = m_scene.reg.try_get<components::Renderable>(m_currentEntity);
            bool render = (rc != nullptr);

            if (ImGui::Checkbox("Render", &render)) {
                if (!render) {
                    m_scene.reg.remove_if_exists<components::Renderable>(m_currentEntity);
                    rc = nullptr;
                } else if (render && !rc) {
                    const auto& model = m_models.front();
                    auto& rc = m_scene.reg.emplace<components::Renderable>(m_currentEntity,
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
                    m_scene.reg.patch<components::Renderable>(m_currentEntity,
                        [&](components::Renderable& r) {
                            const auto& model = m_models[newSelection];
                            r.name = model.name;
                            r.renderable = model.renderable;
                            r.bounds = model.bounds;
                        });
                }
            }
        }

        ImGui::Separator();

        {
            auto plc = m_scene.reg.try_get<components::PointLight>(m_currentEntity);
            bool hasLight = (plc != nullptr);

            if (ImGui::Checkbox("Point light", &hasLight)) {
                if (!hasLight) {
                    m_scene.reg.remove_if_exists<components::PointLight>(m_currentEntity);
                    plc = nullptr;
                } else if (hasLight && !plc) {
                    m_scene.reg.emplace<components::PointLight>(m_currentEntity);
                }
            }

            if (plc) {
                bool changed = false;

                changed |= ImGui::ColorEdit3("Color", &plc->color.x);
                changed |= ImGui::SliderFloat("Linear", &plc->linearAttenuation, 0.0f, 5.0f, "%.5f");
                changed |= ImGui::SliderFloat("Quadratic", &plc->quadraticAttenuation, 0.0f, 5.0f, "%.5f");
                changed |= ImGui::SliderFloat("Intensity", &plc->intensity, 0.0f, 100.0f, "%.5f");

                if (changed) {
                }
            }
        }
    }

    ImGui::End();
}

void SceneEditor::sceneWindow()
{
    if (ImGui::Begin("Scene")) {
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

    m_scene.reg.emplace<components::Misc>(e, fmt::format("{}:{}", model.name, m_scene.reg.size()));
    m_scene.reg.emplace<components::Transform>(e, t);

    m_scene.reg.emplace<components::Renderable>(e, model.name, model.renderable, model.bounds);

    return e;
}

void SceneEditor::mainMenu()
{
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

    XMFLOAT4X4A wm;
    XMStoreFloat4x4A(&wm, t->getMatrix());

    Im3d::Mat4 wm2;
    std::memcpy(&wm2, &wm, sizeof(wm));

    Im3d::PushDrawState();
    Im3d::PushMatrix();
    Im3d::SetMatrix(wm2);
    Im3d::SetSize(3.0f);

    if (const auto rc = std::as_const(m_scene.reg).try_get<components::Renderable>(e)) {
        XMFLOAT3 bMin, bMax;
        XMStoreFloat3(&bMin, rc->bounds.min.vec);
        XMStoreFloat3(&bMax, rc->bounds.max.vec);

        Im3d::DrawAlignedBox(Im3d::Vec3(bMin.x, bMin.y, bMin.z), Im3d::Vec3(bMax.x, bMax.y, bMax.z));
    } else if (const auto plc = std::as_const(m_scene.reg).try_get<components::PointLight>(e)) {
        // TODO: figure out how to calculate the radius
        Im3d::DrawSphere(Im3d::Vec3(0.0f), std::min(plc->intensity, 5.0f));
    }

    Im3d::PopMatrix();
    Im3d::PopDrawState();
}

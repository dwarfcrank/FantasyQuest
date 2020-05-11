#include "pch.h"

#include "SceneEditor.h"
#include "Scene.h"
#include "imgui.h"
#include "DebugDraw.h"
#include "InputMap.h"

SceneEditor::SceneEditor(Scene& scene, InputMap& inputs) :
    GameBase(inputs), m_scene(scene)
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

    doBind(SDLK_r, velocity.y, moveSpeed);
    doBind(SDLK_f, velocity.y, -moveSpeed);

    m_inputs.onMouseMove([this](const SDL_MouseMotionEvent& event) {
        if ((event.state & SDL_BUTTON_RMASK) || (moveCamera)) {
            auto x = static_cast<float>(event.xrel) / 450.0f;
            auto y = static_cast<float>(event.yrel) / 450.0f;
            m_camera.rotate(y, x);
        }
    });
}

bool SceneEditor::update(float dt)
{
    d.clear();

    moveCamera = (SDL_GetModState() & KMOD_LCTRL) != 0;

    drawGrid();

    XMFLOAT3 v(velocity.x * dt, velocity.y * dt, velocity.z * dt);
    float changed = (v.x != 0.0f) || (v.y != 0.0f) || (v.z != 0.0f) || (angle != 0.0f);
    float a = angle * dt;

    if (m_scene.reg.valid(m_currentEntity) && !moveCamera) {
        auto& t = m_scene.reg.get<components::Transform>(m_currentEntity);

		t.position.x += v.x;
		t.position.y += v.y;
		t.position.z += v.z;
		t.rotation.y += a;
    }

    if (moveCamera) {
        m_camera.move(v.x, v.y, v.z);
        m_camera.rotate(0.0f, a);
    }

    sceneWindow();
    objectPropertiesWindow();
    
    return true;
}

void SceneEditor::render(IRenderer* r)
{
    if (!d.verts.empty()) {
        r->debugDraw(m_camera, d.verts);
    }
}

const Camera& SceneEditor::getCamera() const
{
    return m_camera;
}

void SceneEditor::objectList()
{
    entt::entity e = entt::null;

    if (ImGui::BeginChild("##objects", ImVec2(-1.0f, -1.0f))) {
        m_scene.reg.view<components::Misc>()
            .each([&](entt::entity entity, const components::Misc& m) {
				if (ImGui::Selectable(m.name.c_str(), entity == m_currentEntity)) {
					m_currentEntity = entity;
				}
			});
    }

    ImGui::EndChild();
}

void SceneEditor::objectPropertiesWindow()
{
    if (ImGui::Begin("Object")) {
        if (!m_scene.reg.valid(m_currentEntity)) {
            ImGui::Text("No object selected");
            ImGui::End();
            return;
        }

        auto& t = m_scene.reg.get<components::Transform>(m_currentEntity);

        bool changed = false;

        changed |= ImGui::InputFloat3("Position", &t.position.x);
        changed |= ImGui::SliderAngle("X", &t.rotation.x, -180.0f, 180.0f);
        changed |= ImGui::SliderAngle("Y", &t.rotation.y, -180.0f, 180.0f);
        changed |= ImGui::SliderAngle("Z", &t.rotation.z, -180.0f, 180.0f);
        changed |= ImGui::InputFloat3("Scale", &t.scale.x);

        if (changed) {
            // hmmm
        }
    }

    ImGui::End();
}

void SceneEditor::lightList()
{
    if (ImGui::TreeNode("Directional light")) {
        auto& dir = m_scene.directionalLight;

        bool changed = false;

        changed |= ImGui::SliderAngle("X", &dir.x, -180.0f, 180.0f);
        changed |= ImGui::SliderAngle("Y", &dir.y, -180.0f, 180.0f);
        changed |= ImGui::ColorEdit3("Color", &m_scene.directionalLightColor.x);

        ImGui::TreePop();
    }

    for (size_t i = 0; i < m_scene.lights.size(); i++) {
        auto& light = m_scene.lights[i];
        auto id = reinterpret_cast<void*>(i);

        if (ImGui::TreeNode(id, "Point light %lld", i)) {
            bool changed = false;

            changed |= ImGui::InputFloat3("Position", &light.Position.x);
            changed |= ImGui::ColorEdit3("Color", &light.Color.x);
            changed |= ImGui::SliderFloat("Linear", &light.Position.w, 0.0f, 1.0f, "%.5f");
            changed |= ImGui::SliderFloat("Quadratic", &light.Color.w, 0.0f, 1.0f, "%.5f");

            if (changed) {

            }

            ImGui::TreePop();
        }
    }
}

void SceneEditor::sceneWindow()
{
    if (ImGui::Begin("Scene")) {
        if (ImGui::CollapsingHeader("Lights")) {
            lightList();
        }

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Objects")) {
            objectList();
        }

        ImGui::Separator();
    }

    ImGui::End();
}

void SceneEditor::drawGrid()
{
    const XMFLOAT4 gridAxisColor(0.8f, 0.8f, 0.8f, 1.0f);
    const XMFLOAT4 gridColor(0.3f, 0.3f, 0.3f, 1.0f);

    d.drawLine({ -100.0f, 0.0f, 0.0f, 1.0f }, { 100.0f, 0.0f, 0.0f, 1.0f }, gridAxisColor);
    d.drawLine({ 0.0f, 0.0f, -100.0f, 1.0f }, { 0.0f, 0.0f, 100.0f, 1.0f }, gridAxisColor);

    for (auto i = 1; i <= 20; i++) {
        d.drawLine({ float(i) * 5.0f, 0.0f, 100.0f, 1.0f }, { float(i) * 5.0f, 0.0f, -100.0f, 1.0f }, gridColor);
        d.drawLine({ float(i) * -5.0f, 0.0f, 100.0f, 1.0f }, { float(i) * -5.0f, 0.0f, -100.0f, 1.0f }, gridColor);
        d.drawLine({ 100.0f, 0.0f, float(i) * 5.0f, 1.0f }, { -100.0f, 0.0f, float(i) * 5.0f, 1.0f }, gridColor);
        d.drawLine({ 100.0f, 0.0f, float(i) * -5.0f, 1.0f }, { -100.0f, 0.0f, float(i) * -5.0f, 1.0f }, gridColor);
    }
}

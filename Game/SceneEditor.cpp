#include "pch.h"

#include "SceneEditor.h"
#include "Scene.h"
#include "imgui.h"
#include "DebugDraw.h"

SceneEditor::SceneEditor(Scene& scene) :
    m_scene(scene)
{
}

void SceneEditor::update(float dt)
{
    d.clear();
    sceneWindow();
    objectPropertiesWindow();

    if (m_currentObjectIdx >= 0 && m_currentObjectIdx < static_cast<int>(m_scene.objects.size())) {
        d.drawBounds({ -1.0f, -1.0f, -1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f },
            m_scene.objects[m_currentObjectIdx].transform);
        d.drawAABB({ -10.0f, -10.0f, -10.0f, 1.0f }, { 10.0f, 10.0f, 10.0f, 1.0f });
    }
}

void SceneEditor::render(IRenderer* r)
{
}

void SceneEditor::objectList()
{
    int i = 0;

    if (ImGui::BeginChild("##objects", ImVec2(-1.0f, -1.0f))) {
        for (const auto& obj : m_scene.objects) {
            if (ImGui::Selectable(obj.name.c_str(), i == m_currentObjectIdx)) {
                m_currentObjectIdx = i;
            }

            i++;
        }
    }
    ImGui::EndChild();
}

void SceneEditor::objectPropertiesWindow()
{
    if (ImGui::Begin("Object")) {
        if (m_currentObjectIdx < 0 || m_currentObjectIdx >= static_cast<int>(m_scene.objects.size())) {
            ImGui::Text("No object selected");
            ImGui::End();
            return;
        }

        auto& obj = m_scene.objects[m_currentObjectIdx];

        bool changed = false;

        changed |= ImGui::InputFloat3("Position", &obj.position.x);
        changed |= ImGui::SliderAngle("X", &obj.rotation.x, -180.0f, 180.0f);
        changed |= ImGui::SliderAngle("Y", &obj.rotation.y, -180.0f, 180.0f);
        changed |= ImGui::SliderAngle("Z", &obj.rotation.z, -180.0f, 180.0f);
        changed |= ImGui::InputFloat3("Scale", &obj.scale.x);

        if (changed) {
            obj.update();
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

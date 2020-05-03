#include "pch.h"

#include "SceneEditor.h"
#include "Scene.h"
#include "imgui.h"

SceneEditor::SceneEditor(Scene& scene) :
    m_scene(scene)
{
}

void SceneEditor::update(float dt)
{
    sceneWindow();
}

void SceneEditor::render(IRenderer*)
{
}

void SceneEditor::objectList()
{
    for (const auto& obj : m_scene.objects) {
        if (ImGui::TreeNode(obj.name.c_str())) {
            ImGui::Text("ebin");
            ImGui::TreePop();
        }
    }
}

void SceneEditor::lightList()
{
    for (size_t i = 0; i < m_scene.lights.size(); i++) {
        auto& light = m_scene.lights[i];
        auto id = reinterpret_cast<void*>(i);

        if (ImGui::TreeNode(id, "Light %lld", i)) {
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
            //ImGui::TreePop();
        }

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Objects")) {
            objectList();
            //ImGui::TreePop();
        }
    }

    ImGui::End();
}

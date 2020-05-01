#include "pch.h"
#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>
#include <unordered_map>

static Assimp::Importer g_importer;

Mesh::Mesh(const std::filesystem::path& filename)
{
    constexpr auto flags =
        aiProcess_CalcTangentSpace
        | aiProcess_Triangulate
        | aiProcess_ConvertToLeftHanded
        | aiProcess_SortByPType
        | aiProcess_FixInfacingNormals
        | aiProcess_OptimizeMeshes
        | aiProcess_PreTransformVertices;

    auto scene = g_importer.ReadFile(filename.generic_string(), flags);

    if (!scene) {
        throw std::runtime_error(fmt::format("Loading mesh {} failed!", filename.generic_string()));
    }

    std::vector<aiColor3D> materialColors(scene->mNumMaterials);

    for (auto matIdx = 0; matIdx < scene->mNumMaterials; matIdx++) {
        const auto* material = scene->mMaterials[matIdx];
        material->Get(AI_MATKEY_COLOR_DIFFUSE, materialColors[matIdx]);
    }

    // TODO: probably shouldn't just merge different meshes into one...
    u16 baseIdx = 0;
    for (auto meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
        const auto* mesh = scene->mMeshes[meshIdx];

        for (auto i = 0; i < mesh->mNumVertices; i++) {
            const auto& v = mesh->mVertices[i];
            const auto& color = materialColors[mesh->mMaterialIndex];

            m_vertices.push_back(Vertex{
                .Position{v.x, v.y, v.z},
                .Color{color.r, color.g, color.b, 1.0f}
            });
        }

        for (auto faceIdx = 0; faceIdx < mesh->mNumFaces; faceIdx++) {
            const auto& f = mesh->mFaces[faceIdx];

            for (auto i = 0; i < f.mNumIndices; i++) {
                m_indices.push_back(baseIdx + static_cast<u16>(f.mIndices[i]));
            }
        }

        baseIdx = static_cast<u16>(m_indices.size());
    }

    g_importer.FreeScene();
}

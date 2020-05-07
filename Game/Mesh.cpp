#include "pch.h"
#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>
#include <unordered_map>

static Assimp::Importer g_importer;

Mesh::Mesh(const std::filesystem::path& path)
{
    constexpr auto flags =
        aiProcess_CalcTangentSpace
        | aiProcess_GenBoundingBoxes
        | aiProcess_Triangulate
        | aiProcess_ConvertToLeftHanded
        | aiProcess_SortByPType
        | aiProcess_FixInfacingNormals
        | aiProcess_OptimizeMeshes
        | aiProcess_PreTransformVertices;

    auto scene = g_importer.ReadFile(path.generic_string(), flags);

    if (!scene) {
        throw std::runtime_error(fmt::format("Loading mesh {} failed!", path.generic_string()));
    }

    std::vector<aiColor3D> materialColors(scene->mNumMaterials);    

    for (auto matIdx = 0; matIdx < scene->mNumMaterials; matIdx++) {
        const auto* material = scene->mMaterials[matIdx];
        material->Get(AI_MATKEY_COLOR_DIFFUSE, materialColors[matIdx]);
    }

    auto aabbMin = scene->mMeshes[0]->mAABB.mMin;
    auto aabbMax = scene->mMeshes[0]->mAABB.mMax;

    // TODO: probably shouldn't just merge different meshes into one...
    u16 baseIdx = 0;
    for (auto meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
        const auto* mesh = scene->mMeshes[meshIdx];

        aabbMin.x = std::min(aabbMin.x, mesh->mAABB.mMin.x);
        aabbMin.y = std::min(aabbMin.y, mesh->mAABB.mMin.y);
        aabbMin.z = std::min(aabbMin.z, mesh->mAABB.mMin.z);
        aabbMax.x = std::max(aabbMax.x, mesh->mAABB.mMax.x);
        aabbMax.y = std::max(aabbMax.y, mesh->mAABB.mMax.y);
        aabbMax.z = std::max(aabbMax.z, mesh->mAABB.mMax.z);

        for (auto i = 0; i < mesh->mNumVertices; i++) {
            const auto& v = mesh->mVertices[i];
            const auto& n = mesh->mNormals[i];
            const auto& color = materialColors[mesh->mMaterialIndex];

            m_vertices.push_back(Vertex{
                .Position{v.x, v.y, v.z},
                .Normal{n.x, n.y, n.z},
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

    m_bounds.min = Vector<Model>(aabbMin.x, aabbMin.y, aabbMin.z, 1.0f);
    m_bounds.max = Vector<Model>(aabbMax.x, aabbMax.y, aabbMax.z, 1.0f);

    if (const auto& meshName = scene->mMeshes[0]->mName; meshName.length > 0) {
        m_name = scene->mMeshes[0]->mName.C_Str();
    } else {
        m_name = path.filename().generic_string();
    }

    g_importer.FreeScene();
}

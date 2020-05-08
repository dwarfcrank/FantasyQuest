#include "pch.h"
#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>
#include <unordered_map>

static Assimp::Importer g_importer;

template<typename T1, typename T2>
static T1 min3(const T1& a, const T2& b)
{
    T1 r;
    r.x = std::min(a.x, b.x);
    r.y = std::min(a.y, b.y);
    r.z = std::min(a.z, b.z);

    return r;
}

template<typename T1, typename T2>
static T1 max3(const T1& a, const T2& b)
{
    T1 r;
    r.x = std::max(a.x, b.x);
    r.y = std::max(a.y, b.y);
    r.z = std::max(a.z, b.z);

    return r;
}

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

    for (u32 i = 0; i < scene->mNumMaterials; i++) {
        scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, materialColors[i]);
    }

    auto aabbMin = scene->mMeshes[0]->mAABB.mMin;
    auto aabbMax = scene->mMeshes[0]->mAABB.mMax;

    // TODO: probably shouldn't just merge different meshes into one...
    u16 baseIdx = 0;

    for (ArrayView meshes(scene->mMeshes, scene->mNumMeshes); const auto* mesh : meshes) {
        aabbMin = min3(aabbMin, mesh->mAABB.mMin);
        aabbMax = max3(aabbMax, mesh->mAABB.mMax);

        for (u32 i = 0; i < mesh->mNumVertices; i++) {
            const auto& v = mesh->mVertices[i];
            const auto& n = mesh->mNormals[i];
            const auto& color = materialColors[mesh->mMaterialIndex];

            m_vertices.push_back(Vertex{
                .Position{v.x, v.y, v.z},
                .Normal{n.x, n.y, n.z},
                .Color{color.r, color.g, color.b, 1.0f}
            });
        }

        for (ArrayView faces(mesh->mFaces, mesh->mNumFaces); const auto& f : faces) {
            for (ArrayView indices(f.mIndices, f.mNumIndices); auto i : indices) {
                m_indices.push_back(baseIdx + static_cast<u16>(i));
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

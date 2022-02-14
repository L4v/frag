#ifndef MESH_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <map>
#include <iostream>

#include "include/glad/glad.h"

#include "math3d.hpp"

#define POSTPROCESS_FLAGS (aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals)
#define INVALID_MATERIAL 0xFFFFFFFF
#define NUM_BONES_PER_VERTEX 4

struct Material {
    u32 mDiffuseTextureId;
    u32 mSpecularTextureId;
    std::string mDiffusePath;
    std::string mSpecularPath;

    // NOTE(Jovan): Doesn't work without it, even though std::string constructor
    // should be implicitly called
    Material() {
        mDiffusePath = "";
        mSpecularPath = "";
    }
};

struct Mesh {
    u32 mBaseVertex;
    u32 mBaseIndex;
    u32 mNumIndices;
    Material mMaterial;
};

class Model {
private:
    Assimp::Importer mImporter;
    const aiScene *mScene;

    u32 mNumIndices;
public:
    u32 mNumVertices;
    m44 mModel;
    v3 mPosition;
    // TODO(Jovan): Quaternion representation?
    v3 mRotation;
    v3 mScale;
    std::string mFilepath;
    std::string mDirectory;
    std::vector<Mesh> mMeshes;
    Model(const std::string &filePath);
    bool Load(std::vector<v3> &vertices, std::vector<v3> &normals, std::vector<v2> &texCoords, std::vector<u32> &indices);
};

#define MESH_HP
#endif

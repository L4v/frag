#ifndef MESH_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <map>
#include <iostream>

#include "include/glad/glad.h"

#include "shader.hpp"
#include "math3d.hpp"

#define POSTPROCESS_FLAGS (aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals)
#define INVALID_MATERIAL 0xFFFFFFFF
#define NUM_BONES_PER_VERTEX 4

struct Mesh {
    u32 mBaseVertex;
    u32 mBaseIndex;
    u32 mNumIndices;
};

class Model {
private:
    Assimp::Importer mImporter;
    const aiScene *mScene;

    u32 mNumIndices;
public:
    u32 mNumVertices;
    u32 mVAO;
    u32 mBuffers[BUFFER_COUNT];
    m44 mModel;
    v3 mPosition;
    v3 mScale;
    std::string mFilepath;
    std::vector<Mesh> mMeshes;
    Model(const std::string &filePath);
    bool Load();
};

#define MESH_HP
#endif

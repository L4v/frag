#ifndef MESH_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <map>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

#include "include/glad/glad.h"

#include "shader.hpp"

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

    std::vector<Mesh> mMeshes;
    std::vector<r32> mVertices;
    std::vector<u32> mIndices;

    u32 mNumVertices;
    u32 mNumIndices;
public:

    std::string mFilepath;
    Model(const std::string &filePath);
    bool Load();
};

#define MESH_HP
#endif

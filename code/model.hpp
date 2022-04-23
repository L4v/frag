#ifndef MESH_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <map>
#include <iostream>

#include "types.hpp"
#include "include/glad/glad.h"

#include "math3d.hpp"
#include "util.hpp"

#define POSTPROCESS_FLAGS (aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices)
#define INVALID_MATERIAL 0xFFFFFFFF

global const u32 NUM_BONES_PER_VERTEX = 4;

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

struct VertexBoneData {
    u32 mIds[NUM_BONES_PER_VERTEX] = {0};
    r32 mWeights[NUM_BONES_PER_VERTEX] = {0.0f};
    void AddBoneData(u32 id, r32 weight);
};

struct BoneInfos {
    std::vector<m44> mOffsets;
    std::vector<m44> mFinalTransforms;
    u32              mCount = 0;

    void AddBoneInfo(const aiMatrix4x4 &offset);
};

class Model {
private:
    Assimp::Importer mImporter;
    const aiScene *mScene;
    u32 mNumIndices;

    void ReadNodeHierarchy(const aiNode *pNode,const m44 &parentTransform);
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
    std::map<std::string, u32> mBoneNameToIndex;
    std::vector<VertexBoneData> mBones;
    BoneInfos mBoneInfos;


    Model(const std::string &filePath);
    bool Load(std::vector<v3> &vertices, std::vector<v3> &normals, std::vector<v2> &texCoords, std::vector<u32> &indices);
    void LoadBone(u32 globalVertexId, const aiBone *pBone);
    u32 GetBoneId(const aiBone *pBone);
    void LoadBoneTransforms(std::vector<m44> &transforms);

};

#define MESH_HP
#endif

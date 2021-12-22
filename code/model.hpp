#ifndef MESH_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>
#include <vector>
#include <map>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "include/glad/glad.h"

#include "shader.hpp"

#define POSTPROCESS_FLAGS (aiProcess_Triangulate | aiProcess_FlipUVs)
#define INVALID_MATERIAL 0xFFFFFFFF
#define NUM_BONES_PER_VERTEX 4

// TODO(Jovan): Move out? Substitue with material in model context?
struct Texture {
    enum ETextureType {
        DIFFUSE = 0,
        SPECULAR,

        TYPECOUNT
    };

    u32          mId;
    ETextureType mType;
    std::string  mPath;
    Texture(const std::string &path, ETextureType type);
};

struct MeshInfo {
    std::vector<Texture>  mTextures;
    u32                   mNumIndices;
    u32                   mBaseVertex;
    u32                   mBaseIndex;
    u32                   mMaterialIndex;

    MeshInfo();
    void LoadTextures(aiMaterial *material, Texture::ETextureType type, const std::string &dir);

};

struct BoneInfo {
    glm::mat4 mOffset;
    glm::mat4 mFinalTransform;
};

class Model {
private:
    glm::mat4                    mGlobalInverseTransform;
    std::vector<glm::vec3>       mPositions;
    std::vector<glm::vec2>       mTexCoords;
    std::vector<glm::vec3>       mNormals;
    std::vector<u32>             mIndices;
    std::vector<Texture*>        mTextures;
    std::vector<u32>             mBoneIds;
    std::vector<r32>             mBoneWeights;
    std::vector<BoneInfo>        mBoneInfos;
    std::map<std::string, u32>   mBoneMap;

    const aiScene               *mScene;
    u32                          mNumIndices;
    u32                          mNumBones;

    u32 FindPosition(r32 animationTime, const aiNodeAnim* nodeAnimation);
    u32 FindRotation(r32 animationTime, const aiNodeAnim* nodeAnimation);
    u32 FindScaling(r32 animationTime, const aiNodeAnim* nodeAnimation);
    const aiNodeAnim* FindNodeAnimation(const aiAnimation *animation, const std::string &NodeName);
    void InterpolateTranslation(aiVector3D &out, r32 animationTime, const aiNodeAnim *nodeAnimation);
    void InterpolateRotation(aiQuaternion &out, r32 animationTime, const aiNodeAnim *nodeAnimation);
    void InterpolateScaling(aiVector3D &out, r32 animationTime, const aiNodeAnim *nodeAnimation);
    void ReadNodeHierarchy(r32 animationTime, const aiNode *node, const glm::mat4 &parentTransform);
public:
    u32                    mVAO;
    std::vector<MeshInfo>  mMeshes;
    std::vector<GLuint>    mBuffers;
    std::string            mFilename;
    std::string            mDirectory;
    glm::mat4 mModel;
    glm::vec3 mPosition;
    glm::vec3 mRotation;
    glm::vec3 mScale;
    u32                    mNumVertices;

    Model(std::string filename);
    bool Load();
    void ProcessMesh(u32 meshIdx);
    void BoneTransform(r32 timeSeconds, std::vector<glm::mat4> &transforms);

};

#define MESH_HP
#endif

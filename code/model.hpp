#ifndef MESH_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

#define POSTPROCESS_FLAGS (aiProcess_Triangulate | aiProcess_FlipUVs)
#define INVALID_MATERIAL 0xFFFFFFFF

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

class Model {
private:
    std::vector<glm::vec3> mPositions;
    std::vector<glm::vec2> mTexCoords;
    std::vector<glm::vec3> mNormals;
    std::vector<u32>       mIndices;
    std::vector<MeshInfo>  mMeshes;
    std::vector<Texture*>  mTextures;
    std::vector<GLuint>    mBuffers;
    u32                    mVAO;
    u32                    mNumVertices;
    u32                    mNumIndices;

public:
    std::string            mFilename;
    std::string            mDirectory;
    glm::mat4 mModel;
    glm::vec3 mPosition;
    glm::vec3 mRotation;
    glm::vec3 mScale;

    Model(std::string filename);
    bool Load();
    void ProcessMesh(const aiScene *scene, const aiMesh *mesh, u32 meshIdx);
    void Render(const ShaderProgram &program);

};

#define MESH_HP
#endif

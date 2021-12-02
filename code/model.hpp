#ifndef MESH_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define POSTPROCESS_FLAGS (aiProcess_Triangulate | aiProcess_FlipUVs)
#define INVALID_MATERIAL 0xFFFFFFFF

class Material {
public:
    glm::vec3  mAmbient;
    glm::vec3  mDiffuse;
    glm::vec3  mSpecular;
    glm::vec3  mEmission;
    float      mShininess;
};

struct MeshInfo {
    u32      mNumIndices;
    u32      mBaseVertex;
    u32      mBaseIndex;
    u32      mMaterialIndex;
    Material mMaterial;
    bool     mHasTextures;

    MeshInfo();

};

// TODO(Jovan): Add textures
class Texture {

};

class Model {
private:
    std::string            mFilename;
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
    Model(std::string filename);
    bool Load();
    void ProcessMesh(const aiScene *scene, const aiMesh *mesh, u32 meshIdx);
    void Render(const ShaderProgram &program);

};

#define MESH_HP
#endif

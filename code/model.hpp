#ifndef MESH_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define POSTPROCESS_FLAGS (aiProcess_Triangulate | aiProcess_FlipUVs)
#define INVALID_MATERIAL 0xFFFFFFFF

struct MeshInfo {
    u32 mNumIndices;
    u32 mBaseVertex;
    u32 mBaseIndex;
    u32 mMaterialIndex;

    MeshInfo();

};

// TODO(Jovan): Add textures
class Texture {

};

class Model {
private:
    std::string           mFilename;
    std::vector<glm::vec3>      mPositions;
    std::vector<glm::vec2>      mTexCoords;
    std::vector<glm::vec3>      mNormals;
    std::vector<u32>      mIndices;
    std::vector<MeshInfo>     mMeshes;
    std::vector<Texture*> mTextures;
    u32                   mVAO;
    std::vector<GLuint>      mBuffers;
    u32                   mNumVertices;
    u32                   mNumIndices;

public:
    Model(std::string filename);
    bool Load();
    void ProcessMesh(const aiMesh *mesh);
    void Render();

};

#define MESH_HP
#endif

#include "model.hpp"
#include <assimp/Importer.hpp>

MeshInfo::MeshInfo() {
    mNumIndices    = 0;
    mBaseVertex    = 0;
    mBaseIndex     = 0;
    mMaterialIndex = INVALID_MATERIAL;
}

Model::Model(std::string filename) {
    mFilename = filename;
    mNumVertices = 0;
    mNumIndices = 0;
    mBuffers.resize(BUFFER_COUNT);
}

bool
Model::Load() {
    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);
    glGenBuffers(BUFFER_COUNT, &mBuffers[0]);

    bool Result = false;
    Assimp::Importer Importer;

    const aiScene *Scene = Importer.ReadFile(mFilename, POSTPROCESS_FLAGS);

    if(!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode) {
        std::cerr << "[Err] Failed to load model:" << std::endl << Importer.GetErrorString() << std::endl;
        Result = false;
    } else {
        for(u32 MeshIndex = 0; MeshIndex < mMeshes.size(); ++MeshIndex) {
            mMeshes[MeshIndex].mMaterialIndex = Scene->mMeshes[MeshIndex]->mMaterialIndex;
            mMeshes[MeshIndex].mNumIndices = Scene->mMeshes[MeshIndex]->mNumFaces * 3;
            mMeshes[MeshIndex].mBaseVertex = mNumVertices;
            mMeshes[MeshIndex].mBaseIndex = mNumIndices;

            mNumVertices += Scene->mMeshes[MeshIndex]->mNumVertices;
            mNumIndices += mMeshes[MeshIndex].mNumIndices;
        }

        mPositions.reserve(mNumVertices);
        mNormals.reserve(mNumVertices);
        mTexCoords.reserve(mNumVertices);
        mIndices.reserve(mNumIndices);

        for(u32 MeshIndex = 0; MeshIndex < mMeshes.size(); ++MeshIndex) {
            Model::ProcessMesh(Scene->mMeshes[MeshIndex]);
        }
        // TODO(Jovan): Materials (Textures)
        // NOTE(Jovan): Populate buffers
        glBindBuffer(GL_ARRAY_BUFFER, mBuffers[POS_VB]);
        glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(r32) * mPositions.size(), &mPositions[0][0], GL_STATIC_DRAW);
        glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(POSITION_LOCATION);

        // TODO(Jovan): Populate texture coord buffers

        glBindBuffer(GL_ARRAY_BUFFER, mBuffers[NORM_VB]);
        glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(r32) * mPositions.size(), &mPositions[0][0], GL_STATIC_DRAW);
        glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(NORMAL_LOCATION);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBuffers[INDEX_BUFFER]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mIndices[0]) * mIndices.size(), &mIndices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        Result = true;
    }
    
    glBindVertexArray(0);
    return Result;
}

void
Model::ProcessMesh(const aiMesh* mesh) {
    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    for(u32 VertexIndex = 0; VertexIndex < mesh->mNumVertices; ++VertexIndex) {
        const aiVector3D &Position = mesh->mVertices[VertexIndex];
        const aiVector3D &Normal = mesh->mNormals[VertexIndex];
        const aiVector3D &TexCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][VertexIndex] : Zero3D;

        mPositions.push_back(glm::vec3(Position.x, Position.y, Position.z));
        mNormals.push_back(glm::vec3(Normal.x, Normal.y, Normal.z));
        mTexCoords.push_back(glm::vec2(TexCoord.x, TexCoord.y));
    }

    for(u32 FaceIndex = 0; FaceIndex < mesh->mNumFaces; ++FaceIndex) {
        const aiFace& Face = mesh->mFaces[FaceIndex];
        mIndices.push_back(Face.mIndices[0]);
        mIndices.push_back(Face.mIndices[1]);
        mIndices.push_back(Face.mIndices[2]);
    }
}

void
Model::Render() {
    glBindVertexArray(mVAO);
    for(u32 MeshIdx = 0; MeshIdx < mMeshes.size(); ++MeshIdx) {
        // TODO(Jovan): Texture rendering
        glDrawElementsBaseVertex(GL_TRIANGLES,
                mMeshes[MeshIdx].mNumIndices,
                GL_UNSIGNED_INT,
                (void*)(sizeof(u32) * mMeshes[MeshIdx].mBaseIndex),
                mMeshes[MeshIdx].mBaseVertex);
    }
    glBindVertexArray(0);
}

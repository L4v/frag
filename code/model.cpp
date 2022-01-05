#include "model.hpp"
#include <assimp/Importer.hpp>

Model::Model(const std::string &filePath) {
    mFilepath = filePath;
}

bool
Model::Load() {
    
    mScene = mImporter.ReadFile(mFilepath, POSTPROCESS_FLAGS);
    if(!mScene) {
        std::cerr << "[Err] Failed to load " << mFilepath << std::endl;
        return false;
    }

    mMeshes.resize(mScene->mNumMeshes);

    mNumVertices = 0;
    mNumIndices = 0;
    for(u32 MeshIdx = 0; MeshIdx < mScene->mNumMeshes; ++MeshIdx) {
        const aiMesh *Mesh = mScene->mMeshes[MeshIdx];
        // NOTE(Jovan): Triangulated data
        mMeshes[MeshIdx].mNumIndices = Mesh->mNumFaces * 3;
        mMeshes[MeshIdx].mBaseVertex = mNumVertices;
        mMeshes[MeshIdx].mBaseIndex = mNumIndices;

        mNumVertices += Mesh->mNumVertices;
        mNumIndices += mMeshes[MeshIdx].mNumIndices;
    }

    mVertices.reserve(mNumVertices);
    mIndices.reserve(mNumIndices);

    return true;
}
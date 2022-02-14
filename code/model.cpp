#include "model.hpp"
#include <assimp/Importer.hpp>

Model::Model(const std::string &filePath) {
    mFilepath = filePath;
    mDirectory = mFilepath.substr(0, mFilepath.find_last_of(PATH_SEPARATOR));
}

bool
Model::Load(std::vector<v3> &vertices, std::vector<v3> &normals, std::vector<v2> &texCoords, std::vector<u32> &indices) {
    
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

    vertices.reserve(mNumVertices);
    normals.reserve(mNumVertices);
    texCoords.reserve(mNumVertices);
    indices.reserve(mNumIndices);

    for(u32 MeshIdx = 0; MeshIdx < mMeshes.size(); ++MeshIdx) {
        const aiMesh *MeshAI = mScene->mMeshes[MeshIdx];
        const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
        for(u32 VertexIdx = 0; VertexIdx < MeshAI->mNumVertices; ++VertexIdx) {
            const aiVector3D *Vertex = &MeshAI->mVertices[VertexIdx];
            const aiVector3D *Normal = &MeshAI->mNormals[VertexIdx];
            const aiVector3D *TexCoord = MeshAI->HasTextureCoords(0) ? &MeshAI->mTextureCoords[0][VertexIdx] : &Zero3D;

            vertices.push_back(v3(Vertex->x, Vertex->y, Vertex->z));
            normals.push_back(v3(Normal->x, Normal->y, Normal->z));
            texCoords.push_back(v2(TexCoord->x, TexCoord->y));
        }

        for(u32 FaceIdx = 0; FaceIdx < MeshAI->mNumFaces; ++FaceIdx) {
            const aiFace &Face = MeshAI->mFaces[FaceIdx];
            if(Face.mNumIndices != 3) {
                return false;
            }
            indices.push_back(Face.mIndices[0]);
            indices.push_back(Face.mIndices[1]);
            indices.push_back(Face.mIndices[2]);
        }

        const aiMaterial *MaterialAI = mScene->mMaterials[MeshAI->mMaterialIndex];
        u32 DiffuseCount = MaterialAI->GetTextureCount(aiTextureType_DIFFUSE);
        u32 SpecularCount = MaterialAI->GetTextureCount(aiTextureType_SPECULAR);
        aiString PathAI;
 
        if(DiffuseCount && MaterialAI->GetTexture(aiTextureType_DIFFUSE, 0, &PathAI, 0, 0, 0, 0, 0) == AI_SUCCESS) {
            mMeshes[MeshIdx].mMaterial.mDiffusePath = mDirectory + PATH_SEPARATOR + std::string(PathAI.data);
        }

        if(SpecularCount && MaterialAI->GetTexture(aiTextureType_SPECULAR, 0, &PathAI, 0, 0, 0, 0, 0) == AI_SUCCESS) {
            std::string Path = mDirectory + PATH_SEPARATOR + std::string(PathAI.data);
            mMeshes[MeshIdx].mMaterial.mSpecularPath = mDirectory + PATH_SEPARATOR + std::string(PathAI.data);
        }
    }

    return true;
}
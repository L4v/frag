#include "model.hpp"
#include <assimp/Importer.hpp>

Model::Model(const std::string &filePath) {
    mFilepath = filePath;
    mDirectory = mFilepath.substr(0, mFilepath.find_last_of(PATH_SEPARATOR));
}


void
VertexBoneData::AddBoneData(u32 id, r32 weight) {
    for(u32 i = 0; i < ArrayCount(mWeights); ++i) {
        if(!mWeights[i]) {
            mIds[i] = id;
            mWeights[i] = weight;
            std::cout << " bone: " << id << " weight: " << weight << " index: " << id << std::endl;
            return;
        }
    }
}

u32
Model::GetBoneId(const aiBone *pBone) {
    std::string BoneName(pBone->mName.C_Str());
    u32 BoneId = 0;
    if(mBoneNameToIndex.find(BoneName) == mBoneNameToIndex.end()) {
        mBoneNameToIndex[BoneName] = mBoneNameToIndex.size();
    } else {
        BoneId = mBoneNameToIndex[BoneName];
    }

    return BoneId;
}

void
Model::ParseBone(u32 meshId, const aiBone *pBone) {
    u32 BoneId = GetBoneId(pBone);
    for(u32 WeightIdx = 0; WeightIdx < pBone->mNumWeights; ++WeightIdx) {
        if(WeightIdx == 0) {
            std::cout << std::endl;
        }
        const aiVertexWeight &VertWeight = pBone->mWeights[WeightIdx];
        u32 GlobalVertexId = meshId + VertWeight.mVertexId;
        mVertexBoneData[GlobalVertexId].AddBoneData(BoneId, VertWeight.mWeight);
        std::cout << "Vertex: " << GlobalVertexId << " ";
    }
    std::cout << std::endl;
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
        const aiMesh *MeshAI = mScene->mMeshes[MeshIdx];
        Mesh &CurrMesh = mMeshes[MeshIdx];
        // NOTE(Jovan): Triangulated data
        CurrMesh.mNumIndices = MeshAI->mNumFaces * 3;
        CurrMesh.mBaseVertex = mNumVertices;
        CurrMesh.mBaseIndex = mNumIndices;

        mNumVertices += MeshAI->mNumVertices;
        mNumIndices += CurrMesh.mNumIndices;

        mVertexBoneData.resize(mNumVertices);
        if(MeshAI->HasBones()) {
            for(u32 BoneIdx = 0; BoneIdx < MeshAI->mNumBones; ++BoneIdx) {
                ParseBone(CurrMesh.mBaseVertex, MeshAI->mBones[BoneIdx]);
            }
        }
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

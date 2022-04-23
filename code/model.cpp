#include "model.hpp"

Model::Model(const std::string &filePath) {
    mFilepath = filePath;
    mDirectory = mFilepath.substr(0, mFilepath.find_last_of(PATH_SEPARATOR));
}


void
VertexBoneData::AddBoneData(u32 id, r32 weight) {
    for(u32 i = 0; i < ArrayCount(mWeights); ++i) {
        if(mWeights[i] == 0.0f) {
            mIds[i] = id;
            mWeights[i] = weight;
            return;
        }
    }
}

void
BoneInfos::AddBoneInfo(const aiMatrix4x4 &offset) {
    mOffsets.push_back(m44(&offset[0][0]));
    mFinalTransforms.push_back(m44());
    ++mCount;
}

u32
Model::GetBoneId(const aiBone *pBone) {
    u32 BoneId = 0;
    std::string BoneName(pBone->mName.C_Str());
    if(mBoneNameToIndex.find(BoneName) == mBoneNameToIndex.end()) {
        BoneId = mBoneNameToIndex.size();
        mBoneNameToIndex[BoneName] = BoneId;
    } else {
        BoneId = mBoneNameToIndex[BoneName];
    }

    return BoneId;
}

void
Model::LoadBone(u32 baseVertex, const aiBone *pBone) {
    u32 BoneId = GetBoneId(pBone);

    if(BoneId == mBoneInfos.mCount) {
        mBoneInfos.AddBoneInfo(pBone->mOffsetMatrix);
    }

    for(u32 WeightIdx = 0; WeightIdx < pBone->mNumWeights; ++WeightIdx) {
        const aiVertexWeight &VertWeight = pBone->mWeights[WeightIdx];
        u32 GlobalVertexId = baseVertex + VertWeight.mVertexId;
        mBones[GlobalVertexId].AddBoneData(BoneId, VertWeight.mWeight);
    }
}

bool
Model::Load(std::vector<v3> &vertices, std::vector<v3> &normals, std::vector<v2> &texCoords, std::vector<u32> &indices) {
    
    mScene = mImporter.ReadFile(mFilepath, POSTPROCESS_FLAGS);
    if(!mScene) {
        std::cerr << "[Err] Failed to load " << mFilepath << std::endl;
        return false;
    }

    mBoneInfos.mGlobalInverseTransform = ~m44(&mScene->mRootNode->mTransformation[0][0]);

    mMeshes.resize(mScene->mNumMeshes);

    mNumVertices = 0;
    mNumIndices = 0;
    for(u32 MeshIdx = 0; MeshIdx < mScene->mNumMeshes; ++MeshIdx) {
        const aiMesh *MeshAI = mScene->mMeshes[MeshIdx];
        Mesh &CurrMesh = mMeshes[MeshIdx];

        CurrMesh.mNumIndices = MeshAI->mNumFaces * 3;
        CurrMesh.mBaseVertex = mNumVertices;
        CurrMesh.mBaseIndex = mNumIndices;

        mNumVertices += MeshAI->mNumVertices;
        mNumIndices += CurrMesh.mNumIndices;

    }

    vertices.reserve(mNumVertices);
    normals.reserve(mNumVertices);
    texCoords.reserve(mNumVertices);
    mBones.resize(mNumVertices);
    indices.reserve(mNumIndices);

    for(u32 MeshIdx = 0; MeshIdx < mMeshes.size(); ++MeshIdx) {
        const aiMesh *MeshAI = mScene->mMeshes[MeshIdx];
        const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
        Mesh &CurrMesh = mMeshes[MeshIdx];
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

        if(MeshAI->HasBones()) {
            for(u32 BoneIdx = 0; BoneIdx < MeshAI->mNumBones; ++BoneIdx) {
                LoadBone(CurrMesh.mBaseVertex, MeshAI->mBones[BoneIdx]);
            }
        }

        const aiMaterial *MaterialAI = mScene->mMaterials[MeshAI->mMaterialIndex];
        u32 DiffuseCount = MaterialAI->GetTextureCount(aiTextureType_DIFFUSE);
        u32 SpecularCount = MaterialAI->GetTextureCount(aiTextureType_SPECULAR);
        aiString PathAI;
 
        if(DiffuseCount && MaterialAI->GetTexture(aiTextureType_DIFFUSE, 0, &PathAI, 0, 0, 0, 0, 0) == AI_SUCCESS) {
            CurrMesh.mMaterial.mDiffusePath = mDirectory + PATH_SEPARATOR + std::string(PathAI.data);
        }

        if(SpecularCount && MaterialAI->GetTexture(aiTextureType_SPECULAR, 0, &PathAI, 0, 0, 0, 0, 0) == AI_SUCCESS) {
            std::string Path = mDirectory + PATH_SEPARATOR + std::string(PathAI.data);
            CurrMesh.mMaterial.mSpecularPath = mDirectory + PATH_SEPARATOR + std::string(PathAI.data);
        }
    }

    return true;
}

void
Model::LoadBoneTransforms(r32 timeInSeconds, std::vector<m44> &transforms) {
    m44 Identity;
    Identity.LoadIdentity();

    r32 TicksPerSecond = (r32)(mScene->mAnimations[0]->mTicksPerSecond != 0 ?
        mScene->mAnimations[0]->mTicksPerSecond
        : 25.0f);
    r32 TimeInTicks = timeInSeconds * TicksPerSecond;
    r32 AnimationTimeTicks = fmod(TimeInTicks, (r32)(mScene->mAnimations[0]->mDuration));

    ReadNodeHierarchy(mScene->mRootNode, Identity);
    transforms.resize(mBoneInfos.mCount);
    for(u32 BoneInfoIdx = 0; BoneInfoIdx < mBoneInfos.mCount; ++BoneInfoIdx) {
        transforms[BoneInfoIdx] = mBoneInfos.mFinalTransforms[BoneInfoIdx];
    }
}

m44
Model::CalcInterpolatedScaling(r32 animationTimeInTicks, const aiNodeAnim *pNodeAnim) {
    if(pNodeAnim->mNumScalingKeys == 1) {
        return m44(1.0f).Scale(v3(pNodeAnim->mScalingKeys[0].mValue.x, pNodeAnim->mScalingKeys[0].mValue.y, pNodeAnim->mScalingKeys[0].mValue.z));
    }

    u32 ScalingIndex = FindScaling(animationTimeInTicks, pNodeAnim);
    u32 NextScalingIndex = ScalingIndex + 1;
    r32 T1 = (r32)pNodeAnim->mScalingKeys[ScalingIndex].mTime
    // TODO(Jovan): Continue from here 10:22 https://www.youtube.com/watch?v=gnnoPaStVzg
}

void
Model::ReadNodeHierarchy(r32 animationTimeInTicks, const aiNode *pNode, const m44 &parentTransform) {
    std::string Name(pNode->mName.data);

    const aiAnimation *pAnimation = mScene->mAnimations[0];
    m44 NodeTransform(&pNode->mTransformation[0][0]);
    m44 GlobalTransform = parentTransform * NodeTransform;
    const aiNodeAnim *pNodeAnim = FindNodeAnim(pAnimation, Name);

    if(aiNodeAnim) {
        CalcInterpolatedScaling(animationTimeInTicks, pNodeAnim);
    }

    if(mBoneNameToIndex.find(Name) != mBoneNameToIndex.end()) {
        u32 BoneIndex = mBoneNameToIndex[Name];
        mBoneInfos.mFinalTransforms[BoneIndex] = mBoneInfos.mGlobalInverseTransform * GlobalTransform * mBoneInfos.mOffsets[BoneIndex];
    }

    for(u32 ChildIdx = 0; ChildIdx < pNode->mNumChildren; ++ChildIdx) {
        ReadNodeHierarchy(pNode->mChildren[ChildIdx], GlobalTransform);
    }
}

aiNodeAnim*
Model::FindNodeAnim(const aiAnimation *pAnimation, const std::string &nodeName) {
    // TODO(Jovan): Do via map?
    for(u32 ChannelIdx = 0; ChannelIdx < pAnimation->mNumChannels; ++ChannelIdx) {
        aiNodeAnim *pNodeAnim = pAnimation->mChannels[ChannelIdx];

        if(std::string(pNodeAnim->mNodeName.data) == nodeName) {
            return pNodeAnim;
        }
    }

    return 0;
}


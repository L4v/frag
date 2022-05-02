#include "model.hpp"

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
    mFinalTransforms.push_back(m44(1.0f));
    ++mCount;
}

Model::Model(const std::string &filePath) {
    mFilepath = filePath;
    mDirectory = mFilepath.substr(0, mFilepath.find_last_of(PATH_SEPARATOR));
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
    r32 AnimationTimeInTicks = fmod(TimeInTicks, (r32)(mScene->mAnimations[0]->mDuration));

    ReadNodeHierarchy(AnimationTimeInTicks, mScene->mRootNode, Identity);
    transforms.resize(mBoneInfos.mCount);
    for(u32 BoneInfoIdx = 0; BoneInfoIdx < mBoneInfos.mCount; ++BoneInfoIdx) {
        transforms[BoneInfoIdx] = mBoneInfos.mFinalTransforms[BoneInfoIdx];
    }
}

u32
Model::FindScaling(r32 animationTimeInTicks, const aiNodeAnim *pNodeAnim) {
    for(u32 ScalingIdx = 0; ScalingIdx < pNodeAnim->mNumScalingKeys - 1; ++ScalingIdx) {
        r32 T = (r32)pNodeAnim->mScalingKeys[ScalingIdx + 1].mTime;
        if(animationTimeInTicks < T) {
            return ScalingIdx;
        }
    }

    return 0;
}

u32
Model::FindPosition(r32 animationTimeInTicks, const aiNodeAnim *pNodeAnim) {
    for(u32 PositionIdx = 0; PositionIdx < pNodeAnim->mNumPositionKeys - 1; ++PositionIdx) {
        r32 T = (r32)pNodeAnim->mPositionKeys[PositionIdx + 1].mTime;
        if(animationTimeInTicks < T) {
            return PositionIdx;
        }
    }

    return 0;
}

u32
Model::FindRotation(r32 animationTimeInTicks, const aiNodeAnim *pNodeAnim) {
    for(u32 RotationIdx = 0; RotationIdx < pNodeAnim->mNumRotationKeys - 1; ++RotationIdx) {
        r32 T = (r32)pNodeAnim->mRotationKeys[RotationIdx + 1].mTime;
        if(animationTimeInTicks < T) {
            return RotationIdx;
        }
    }

    return 0;
}

v3
Model::CalcInterpolatedScaling(r32 animationTimeInTicks, const aiNodeAnim *pNodeAnim) {
    if(pNodeAnim->mNumScalingKeys == 1) {
        return v3(pNodeAnim->mScalingKeys[0].mValue);
    }

    u32 ScalingIndex = FindScaling(animationTimeInTicks, pNodeAnim);
    u32 NextScalingIndex = ScalingIndex + 1;
    aiVectorKey StartKey = pNodeAnim->mScalingKeys[ScalingIndex];
    aiVectorKey EndKey = pNodeAnim->mScalingKeys[NextScalingIndex];
    r32 T1 = StartKey.mTime;
    r32 T2 = EndKey.mTime;
    r32 DeltaTime = T2 - T1;
    r32 Factor = (animationTimeInTicks - T1) / DeltaTime;
    v3 A(StartKey.mValue);
    v3 B(EndKey.mValue);
    return Lerp(A, B, Factor);
}

v3
Model::CalcInterpolatedPosition(r32 animationTimeInTicks, const aiNodeAnim *pNodeAnim) {
    if(pNodeAnim->mNumPositionKeys == 1) {
        return v3(pNodeAnim->mPositionKeys[0].mValue);
    }

    u32 PositionIndex = FindPosition(animationTimeInTicks, pNodeAnim);
    u32 NextPositionIndex = PositionIndex + 1;
    aiVectorKey StartKey = pNodeAnim->mPositionKeys[PositionIndex];
    aiVectorKey EndKey = pNodeAnim->mPositionKeys[NextPositionIndex];
    r32 T1 = StartKey.mTime;
    r32 T2 = EndKey.mTime;
    r32 DeltaTime = T2 - T1;
    r32 Factor = (animationTimeInTicks - T1) / DeltaTime;
    v3 A(StartKey.mValue);
    v3 B(EndKey.mValue);
    return Lerp(A, B, Factor);
}

quat
Model::CalcInterpolatedRotation(r32 animationTimeInTicks, const aiNodeAnim *pNodeAnim) {
    if(pNodeAnim->mNumRotationKeys == 1) {
        return quat(pNodeAnim->mRotationKeys[0].mValue);
    }

    u32 RotationIndex = FindRotation(animationTimeInTicks, pNodeAnim);
    u32 NextRotationIndex = RotationIndex + 1;
    aiQuatKey StartKey = pNodeAnim->mRotationKeys[RotationIndex];
    aiQuatKey EndKey = pNodeAnim->mRotationKeys[NextRotationIndex];
    r32 T1 = StartKey.mTime;
    r32 T2 = EndKey.mTime;
    r32 DeltaTime = T2 - T1;
    r32 Factor = (animationTimeInTicks - T1) / DeltaTime;
    aiQuaternion &A = StartKey.mValue;
    aiQuaternion &B = EndKey.mValue;
    aiQuaternion Result;
    aiQuaternion::Interpolate(Result, A, B, Factor);
    Result = Result.Normalize();
    return quat(Result);
}


void
Model::ReadNodeHierarchy(r32 animationTimeInTicks, const aiNode *pNode, const m44 &parentTransform) {
    std::string Name(pNode->mName.data);

    const aiAnimation *pAnimation = mScene->mAnimations[0];
    m44 NodeTransform(&pNode->mTransformation[0][0]);
    const aiNodeAnim *pNodeAnim = FindNodeAnim(pAnimation, Name);

    if(pNodeAnim) {
        v3 Scaling = CalcInterpolatedScaling(animationTimeInTicks, pNodeAnim);
        v3 Position = CalcInterpolatedPosition(animationTimeInTicks, pNodeAnim);
        quat Rotation = CalcInterpolatedRotation(animationTimeInTicks, pNodeAnim);
        m44 ScalingM = m44(1.0f).Scale(Scaling);
        m44 TranslationM = m44(1.0f).Translate(Position);
        m44 RotationM = m44(Rotation);
        NodeTransform = NodeTransform.LoadIdentity();
    }

    m44 GlobalTransform = parentTransform * NodeTransform;

    if(mBoneNameToIndex.find(Name) != mBoneNameToIndex.end()) {
        u32 BoneIndex = mBoneNameToIndex[Name];
        mBoneInfos.mFinalTransforms[BoneIndex] = mBoneInfos.mGlobalInverseTransform * GlobalTransform * mBoneInfos.mOffsets[BoneIndex];
    }

    for(u32 ChildIdx = 0; ChildIdx < pNode->mNumChildren; ++ChildIdx) {
        ReadNodeHierarchy(animationTimeInTicks, pNode->mChildren[ChildIdx], GlobalTransform);
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


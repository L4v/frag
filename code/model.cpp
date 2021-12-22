#include "model.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

Texture::Texture(const std::string &path, ETextureType type) {
    mPath = path;
    mType = type;
    std::replace(mPath.begin(), mPath.end(), '\\', '/');
    glGenTextures(1, &mId);
    i32 Width, Height, Channels;
    unsigned char *Data = stbi_load(mPath.c_str(), &Width, &Height, &Channels, 0);
    std::cout << "Loading " << mPath << " image" << std::endl;
    if(Data) {
        GLenum Format;
        if(Channels == 1) {
            Format = GL_RED;
        } else if (Channels == 3) {
            Format = GL_RGB;
        } else if (Channels == 4) {
            Format = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, mId);
        glTexImage2D(GL_TEXTURE_2D, 0, Format, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        stbi_image_free(Data);
    } else {
        std::cerr << "[Err] Texture failed to load" << std::endl;
    }
}

MeshInfo::MeshInfo() {
    mNumIndices    = 0;
    mBaseVertex    = 0;
    mBaseIndex     = 0;
    mMaterialIndex = INVALID_MATERIAL;
}

void
MeshInfo::LoadTextures(aiMaterial *material, Texture::ETextureType type, const std::string &dir) {
    // TODO(Jovan): Map better
    aiTextureType aiType = type == Texture::DIFFUSE ? aiTextureType_DIFFUSE : aiTextureType_SPECULAR;
    for(u32 TexIdx = 0; TexIdx < material->GetTextureCount(aiType); ++TexIdx) {
        aiString TexName;
        material->GetTexture(aiType, TexIdx, &TexName);
        // TODO(Jovan): Check for duplicates
        Texture Tex(dir + '/' + TexName.C_Str(), type);
        mTextures.push_back(Tex);
    }
}

Model::Model(std::string filename) {
    mFilename = filename;
    mDirectory = filename.substr(0, filename.find_last_of('/'));
    mNumVertices = 0;
    mNumIndices = 0;
    mBuffers.resize(BUFFER_COUNT);
}

// TODO(Jovan): Extract buffers as structs
struct Buffer {
    GLuint     mId;
    GLenum     mType;
    GLsizei    mCount;
};

struct Buffers {
    u32        mCount;
    Buffer    *mBuffers;
};

//TODO(Jovan): Move out
internal inline glm::mat4
aiMatrix4x4ToGLM(const aiMatrix4x4 &aiMat) {
    return glm::transpose(glm::make_mat4(&aiMat.a1));
}

bool
Model::Load() {
    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);
    glGenBuffers(BUFFER_COUNT, &mBuffers[0]);

    bool Result = false;
    Assimp::Importer Importer;

    mScene = Importer.ReadFile(mFilename, POSTPROCESS_FLAGS);

    if(!mScene || mScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !mScene->mRootNode) {
        std::cerr << "[Err] Failed to load model:" << std::endl << Importer.GetErrorString() << std::endl;
        Result = false;
    } else {
        aiMatrix4x4 Transform = mScene->mRootNode->mTransformation;
        Transform.Inverse();
        mGlobalInverseTransform = aiMatrix4x4ToGLM(Transform);

        mMeshes.resize(mScene->mNumMeshes);
        mTextures.resize(mScene->mNumMaterials);
        for(u32 MeshIndex = 0; MeshIndex < mMeshes.size(); ++MeshIndex) {
            mMeshes[MeshIndex].mMaterialIndex = mScene->mMeshes[MeshIndex]->mMaterialIndex;
            mMeshes[MeshIndex].mNumIndices = mScene->mMeshes[MeshIndex]->mNumFaces * 3;
            mMeshes[MeshIndex].mBaseVertex = mNumVertices;
            mMeshes[MeshIndex].mBaseIndex = mNumIndices;

            mNumVertices += mScene->mMeshes[MeshIndex]->mNumVertices;
            mNumIndices += mMeshes[MeshIndex].mNumIndices;
        }

        mPositions.reserve(mNumVertices);
        mNormals.reserve(mNumVertices);
        mTexCoords.reserve(mNumVertices);
        mIndices.reserve(mNumIndices);

        // NOTE(Jovan): Should be zeroed
        mBoneIds.resize(mNumVertices);
        mBoneWeights.resize(mNumVertices);

        for(u32 MeshIdx = 0; MeshIdx < mMeshes.size(); ++MeshIdx) {
            Model::ProcessMesh(MeshIdx);
        }
        // NOTE(Jovan): Populate buffers
        glBindBuffer(GL_ARRAY_BUFFER, mBuffers[POS_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mPositions[0]) * mPositions.size(), &mPositions[0], GL_STATIC_DRAW);
        glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(POSITION_LOCATION);

        glBindBuffer(GL_ARRAY_BUFFER, mBuffers[TEXCOORD_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mTexCoords[0]) * mTexCoords.size(), &mTexCoords[0], GL_STATIC_DRAW);
        glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(TEX_COORD_LOCATION);

        glBindBuffer(GL_ARRAY_BUFFER, mBuffers[NORM_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mNormals[0]) * mNormals.size(), &mNormals[0], GL_STATIC_DRAW);
        glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(NORMAL_LOCATION);

        glBindBuffer(GL_ARRAY_BUFFER, mBuffers[BONE_ID_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mBoneIds[0]) * mBoneIds.size(), &mBoneIds[0], GL_STATIC_DRAW);
        glVertexAttribIPointer(BONE_ID_LOCATION, NUM_BONES_PER_VERTEX, GL_INT, 0, 0);
        glEnableVertexAttribArray(BONE_ID_LOCATION);

        glBindBuffer(GL_ARRAY_BUFFER, mBuffers[BONE_WEIGHT_VB]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mBoneWeights[0]) * mBoneWeights.size(), &mBoneWeights[0], GL_STATIC_DRAW);
        glVertexAttribPointer(BONE_WEIGHT_LOCATION, NUM_BONES_PER_VERTEX, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(BONE_WEIGHT_LOCATION);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBuffers[INDEX_BUFFER]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mIndices[0]) * mIndices.size(), &mIndices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        std::cout << "Loaded " << mMeshes.size() << " meshes" << std::endl;

        Result = true;
    }
    
    glBindVertexArray(0);
    return Result;
}

void
Model::ProcessMesh(u32 meshIdx) {
    const aiMesh *Mesh = mScene->mMeshes[meshIdx];
    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    for(u32 VertexIndex = 0; VertexIndex < Mesh->mNumVertices; ++VertexIndex) {
        const aiVector3D &TexCoord = Mesh->HasTextureCoords(0) ? Mesh->mTextureCoords[0][VertexIndex] : Zero3D;

        mPositions.push_back(glm::vec3(Mesh->mVertices[VertexIndex].x, Mesh->mVertices[VertexIndex].y, Mesh->mVertices[VertexIndex].z));
        mNormals.push_back(glm::vec3(Mesh->mNormals[VertexIndex].x, Mesh->mNormals[VertexIndex].y, Mesh->mNormals[VertexIndex].z));
        mTexCoords.push_back(glm::vec2(TexCoord.x, TexCoord.y));
    }

    for(u32 FaceIndex = 0; FaceIndex < Mesh->mNumFaces; ++FaceIndex) {
        const aiFace& Face = Mesh->mFaces[FaceIndex];
        mIndices.push_back(Face.mIndices[0]);
        mIndices.push_back(Face.mIndices[1]);
        mIndices.push_back(Face.mIndices[2]);
    }

    // NOTE(Jovan): Load material data
    aiMaterial *MeshMaterial = mScene->mMaterials[Mesh->mMaterialIndex];

    if(Mesh->mMaterialIndex >= 0) {
        std::cout << "Diffuse count: " << MeshMaterial->GetTextureCount(aiTextureType_DIFFUSE) << std::endl;
        std::cout << "Specular count: " << MeshMaterial->GetTextureCount(aiTextureType_SPECULAR) << std::endl;
        mMeshes[meshIdx].LoadTextures(MeshMaterial, Texture::DIFFUSE, mDirectory);
        mMeshes[meshIdx].LoadTextures(MeshMaterial, Texture::SPECULAR, mDirectory);
    }

    //NOTE(Jovan): Load bones
    for(u32 BoneIdx = 0; BoneIdx < Mesh->mNumBones; ++BoneIdx) {
        u32 BoneIndex = 0;
        std::string BoneName(Mesh->mBones[BoneIdx]->mName.data);

        if(mBoneMap.find(BoneName) == mBoneMap.end()) {
            BoneIndex = mNumBones++;
            BoneInfo Info;
            mBoneInfos.push_back(Info);
        } else {
            // TODO(Jovan): ???
            BoneIndex = mBoneMap[BoneName];
        }
        mBoneMap[BoneName] = BoneIndex;
        mBoneInfos[BoneIndex].mOffset = aiMatrix4x4ToGLM(Mesh->mBones[BoneIdx]->mOffsetMatrix);

        for(u32 WeightIdx = 0; WeightIdx < Mesh->mBones[BoneIdx]->mNumWeights; ++WeightIdx) {
            u32 VertexId = mMeshes[meshIdx].mBaseVertex + Mesh->mBones[BoneIdx]->mWeights[WeightIdx].mVertexId;
            r32 Weight = Mesh->mBones[BoneIdx]->mWeights[WeightIdx].mWeight;
            // NOTE(Jovan): Inserting in parallel for SIMD
            for(u32 i = 0; i < NUM_BONES_PER_VERTEX; ++i) {
                if(mBoneWeights[VertexId + i] == 0.0f) {
                    mBoneIds[VertexId + i] = BoneIndex;
                    mBoneWeights[VertexId + i] = Weight;
                    break;
                }
            }
        }
    }
}

void
Model::BoneTransform(r32 timeInSeconds, std::vector<glm::mat4> &transforms) {
    glm::mat4 Identity = glm::mat4(1.0f);
    r32 TicksPerSecond = (r32)(mScene->mAnimations[0]->mDuration);
    r32 TimeInTicks = timeInSeconds * TicksPerSecond;
    r32 AnimationTime = fmod(TimeInTicks, (r32)mScene->mAnimations[0]->mDuration);

    ReadNodeHierarchy(AnimationTime, mScene->mRootNode, Identity);

    transforms.resize(mNumBones);
    for(u32 BoneIdx = 0; BoneIdx < mNumBones; ++BoneIdx) {
        transforms[BoneIdx] = mBoneInfos[BoneIdx].mFinalTransform;
    }
}

void
Model::ReadNodeHierarchy(r32 animationTime, const aiNode *node, const glm::mat4 &parentTransform) {
    std::string NodeName(node->mName.data);
    const aiAnimation *animation = mScene->mAnimations[0];
    glm::mat4 NodeTransform = aiMatrix4x4ToGLM(node->mTransformation);
    const aiNodeAnim *nodeAnimation = FindNodeAnimation(animation, NodeName);

    if(nodeAnimation) {
        aiVector3D Scaling;
        InterpolateScaling(Scaling, animationTime, nodeAnimation);
        glm::mat4 ScalingM = glm::scale(glm::mat4(1.0f), glm::vec3(Scaling.x, Scaling.y, Scaling.z));

        aiQuaternion Rotation;
        InterpolateRotation(Rotation, animationTime, nodeAnimation);
        glm::mat4 RotationM = glm::mat4(1.0f);
        RotationM = glm::rotate(RotationM, glm::radians(Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        RotationM = glm::rotate(RotationM, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        RotationM = glm::rotate(RotationM, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        aiVector3D Translation;
        InterpolateTranslation(Translation, animationTime, nodeAnimation);
        glm::mat4 TranslationM = glm::translate(glm::mat4(1.0f), glm::vec3(Translation.x, Translation.y, Translation.z));

        NodeTransform = TranslationM * RotationM * ScalingM;
    }

    glm::mat4 GlobalTransform = parentTransform * NodeTransform;
    if(mBoneMap.find(NodeName) != mBoneMap.end()) {
        u32 BoneIndex = mBoneMap[NodeName];
        mBoneInfos[BoneIndex].mFinalTransform = mGlobalInverseTransform * GlobalTransform * mBoneInfos[BoneIndex].mOffset;
    }

    for(u32 ChildIdx = 0; ChildIdx < node->mNumChildren; ++ChildIdx) {
        ReadNodeHierarchy(animationTime, node->mChildren[ChildIdx], GlobalTransform);
    }
}

void
Model::InterpolateTranslation(aiVector3D &out, r32 animationTime, const aiNodeAnim *nodeAnimation) {
    if(nodeAnimation->mNumPositionKeys == 1) {
        out = nodeAnimation->mPositionKeys[0].mValue;
        return;
    }

    u32 PositionIndex = FindPosition(animationTime, nodeAnimation);
    u32 NextPositionIndex = PositionIndex + 1;
    r32 T1 = (r32)nodeAnimation->mPositionKeys[PositionIndex].mTime - (r32)nodeAnimation->mPositionKeys[0].mTime;
    r32 T2 = (r32)nodeAnimation->mPositionKeys[NextPositionIndex].mTime - (r32)nodeAnimation->mPositionKeys[0].mTime;
    r32 DT = T2 - T1;
    r32 Factor = (animationTime - T1) / DT;
    const aiVector3D &Start = nodeAnimation->mPositionKeys[PositionIndex].mValue;
    const aiVector3D &End = nodeAnimation->mPositionKeys[NextPositionIndex].mValue;
    aiVector3D Delta = End - Start;
    out = Start + Factor * Delta;
}

void
Model::InterpolateScaling(aiVector3D &out, r32 animationTime, const aiNodeAnim *nodeAnimation) {
    if(nodeAnimation->mNumScalingKeys == 1) {
        out = nodeAnimation->mScalingKeys[0].mValue;
        return;
    }

    u32 ScalingIndex = FindScaling(animationTime, nodeAnimation);
    u32 NextScalingIndex = ScalingIndex + 1;
    r32 T1 = (r32)nodeAnimation->mScalingKeys[ScalingIndex].mTime - (r32)nodeAnimation->mScalingKeys[0].mTime;
    r32 T2 = (r32)nodeAnimation->mScalingKeys[NextScalingIndex].mTime - (r32)nodeAnimation->mScalingKeys[0].mTime;
    r32 DT = T2 - T1;
    r32 Factor = (animationTime - T1) / DT;
    const aiVector3D &Start = nodeAnimation->mScalingKeys[ScalingIndex].mValue;
    const aiVector3D &End = nodeAnimation->mScalingKeys[NextScalingIndex].mValue;
    aiVector3D Delta = End - Start;
    out = Start + Factor * Delta;
}

void
Model::InterpolateRotation(aiQuaternion &out, r32 animationTime, const aiNodeAnim *nodeAnimation) {
    if(nodeAnimation->mNumRotationKeys == 1) {
        out = nodeAnimation->mRotationKeys[0].mValue;
        return;
    }

    u32 RotationIndex = FindRotation(animationTime, nodeAnimation);
    u32 NextRotationIndex = RotationIndex + 1;
    r32 T1 = (r32)nodeAnimation->mRotationKeys[RotationIndex].mTime - (r32)nodeAnimation->mScalingKeys[0].mTime;
    r32 T2 = (r32)nodeAnimation->mRotationKeys[NextRotationIndex].mTime - (r32)nodeAnimation->mScalingKeys[0].mTime;
    r32 DT = T2 - T1;
    r32 Factor = (animationTime - T1) / DT;
    const aiQuaternion &Start = nodeAnimation->mRotationKeys[RotationIndex].mValue;
    const aiQuaternion &End = nodeAnimation->mRotationKeys[NextRotationIndex].mValue;
    aiQuaternion::Interpolate(out, Start, End, Factor);
    out.Normalize();
}

u32
Model::FindPosition(r32 animationTime, const aiNodeAnim* nodeAnimation) {
    for(u32 KeyIdx = 0; KeyIdx < nodeAnimation->mNumPositionKeys - 1; ++KeyIdx) {
        r32 T = (r32)nodeAnimation->mPositionKeys[KeyIdx + 1].mTime - (r32)nodeAnimation->mPositionKeys[0].mTime;
        if(animationTime < T) {
            return KeyIdx;
        }
    }
    std::cerr << "[Err] Should not reach this" << std::endl;
    return 0;
}

u32
Model::FindRotation(r32 animationTime, const aiNodeAnim* nodeAnimation) {
    for(u32 KeyIdx = 0; KeyIdx < nodeAnimation->mNumRotationKeys - 1; ++KeyIdx) {
        r32 T = (r32)nodeAnimation->mRotationKeys[KeyIdx + 1].mTime - (r32)nodeAnimation->mRotationKeys[0].mTime;
        if(animationTime < T) {
            return KeyIdx;
        }
    }
    std::cerr << "[Err] Should not reach this" << std::endl;
    return 0;
}

u32
Model::FindScaling(r32 animationTime, const aiNodeAnim* nodeAnimation) {
    for(u32 KeyIdx = 0; KeyIdx < nodeAnimation->mNumScalingKeys - 1; ++KeyIdx) {
        r32 T = (r32)nodeAnimation->mScalingKeys[KeyIdx + 1].mTime - (r32)nodeAnimation->mScalingKeys[0].mTime;
        if(animationTime < T) {
            return KeyIdx;
        }
    }
    std::cerr << "[Err] Should not reach this" << std::endl;
    return 0;
}


const aiNodeAnim*
Model::FindNodeAnimation(const aiAnimation *animation, const std::string &nodeName) {
    for(u32 ChannelIdx = 0; ChannelIdx < animation->mNumChannels; ++ChannelIdx) {
        const aiNodeAnim *NodeAnimation = animation->mChannels[ChannelIdx];
        if(std::string(NodeAnimation->mNodeName.data) == nodeName) {
            return NodeAnimation;
        }
    }
    return NULL;
}

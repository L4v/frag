#include "model.hpp"
#include "shader.hpp"
#include "types.hpp"
#include <algorithm>
#include <cstring>
#include <iterator>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/tiny_gltf.h"

GLTFModel::Texture::Texture() {
    mWidth = 0.0f;
    mHeight = 0.0f;
}

GLTFModel::Texture::Texture(r32 width, r32 height, const u8 *data) {
    mWidth = width;
    mHeight = height;

    // TODO(Jovan): Tidy up, parameterize and move out / (reuse / refactor) existing 
    glGenTextures(1, &mId);
    glBindTexture(GL_TEXTURE_2D, mId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
}

void
GLTFModel::KeyframesV3::Load(const r32 *timesData, const r32 *valuesData) {
    mValues.resize(mCount);
    mTimes.resize(mCount);
    memcpy(mTimes.data(), timesData, mCount * sizeof(r32));
    memcpy(mValues.data(), valuesData, mCount * 3 * sizeof(r32));
}

v3
GLTFModel::KeyframesV3::Interpolate(r64 timeInSeconds) {
    if(mTimes.size() == 1) {
        return mValues[0];
    }

    u32 StartIdx = 0;
    u32 EndIdx = 0;

    for(u32 i = 0; i < mTimes.size() - 1; ++i) {
        if(mTimes[i + 1] > timeInSeconds) {
            break;
        }
        StartIdx = i;
    }

    EndIdx = StartIdx + 1;
    r32 T1 = mTimes[StartIdx];
    r32 T2 = mTimes[EndIdx];
    v3 Begin = mValues[StartIdx];
    v3 End = mValues[EndIdx];
    r32 Factor = (timeInSeconds - T1) / (T2 - T1);

    return Lerp(Begin, End, Factor);
}

void
GLTFModel::KeyframesQuat::Load(const r32 *timesData, const r32 *valuesData) {
    mValues.resize(mCount);
    mTimes.resize(mCount);
    memcpy(mTimes.data(), timesData, mCount * sizeof(r32));
    memcpy(mValues.data(), valuesData, mCount * 4 * sizeof(r32));
}

quat
GLTFModel::KeyframesQuat::Interpolate(r64 timeInSeconds) {
    if(mTimes.size() == 1) {
        return mValues[0];
    }

    u32 StartIdx = 0;
    u32 EndIdx = 0;

    for(u32 i = 0; i < mTimes.size() - 1; ++i) {
        if(mTimes[i + 1] > timeInSeconds) {
            break;
        }
        StartIdx = i;
    }

    EndIdx = StartIdx + 1;
    r32 T1 = mTimes[StartIdx];
    r32 T2 = mTimes[EndIdx];
    quat Begin = mValues[StartIdx];
    quat End = mValues[EndIdx];
    r32 Factor = (timeInSeconds - T1) / (T2 - T1);
    quat Slerped = Slerp(Begin, End, Factor);
    return Slerped.GetNormalized();
}

void
GLTFModel::Keyframes::Load(const std::string &path, u32 count, const r32 *timesData, const r32 *valuesData) {
    if(path == "translation") {
        mTranslation.mCount = count;
        mTranslation.Load(timesData, valuesData);
    } else if (path == "rotation") {
        mRotation.mCount = count;
        mRotation.Load(timesData, valuesData);
    } else if (path == "scale") {
        mScale.mCount = count;
        mRotation.Load(timesData, valuesData);
    }
}

GLTFModel::Animation::Animation(i32 idx) {
    mIdx = idx;
    mDurationInSeconds = 0.0f;
}

GLTFModel::GLTFModel(const std::string &filePath) {
    tinygltf::Model tinyModel;
    loadModel(&tinyModel, filePath);
    loadNodes(&tinyModel);
    loadAnimations(&tinyModel);
}

void
GLTFModel::loadModel(tinygltf::Model *tinyModel, const std::string &filePath) {
    mFilePath = filePath;
    std::string fileExtension = filePath.substr(filePath.find_last_of(".") + 1, filePath.length());
    tinygltf::TinyGLTF Loader;
    std::string Err;
    std::string Warn;
    bool Ret = false;

    if(fileExtension == "gltf") {
        Ret = Loader.LoadASCIIFromFile(tinyModel, &Err, &Warn, filePath);
    } else {
        Ret = Loader.LoadBinaryFromFile(tinyModel, &Err, &Warn, filePath);
    }

    if(!Err.empty()) {
        std::cerr << "Error: " << Err << std::endl;
        return;
    }

    if(!Warn.empty()) {
        std::cerr << "Warning: " << Warn << std::endl;
        return;
    }

    if(!Ret) {
        std::cerr << "Error: Failed to load model" << std::endl;
        return;
    }

    for(const auto &Buffer : tinyModel->buffers) {
        mData.insert(std::end(mData), std::begin(Buffer.data), std::end(Buffer.data));
    }
}

void
GLTFModel::loadNodes(tinygltf::Model *tinyModel) {
        // NOTE(Jovan): Load all inverse bind pose matrices
        const tinygltf::Skin &Skin = tinyModel->skins[0];
        std::vector<r32> InverseBindPoseReals = LoadFloats(tinyModel, Skin.inverseBindMatrices);

        for(u32 i = 0; i < InverseBindPoseReals.size(); i += 16) {
            m44 InverseBindPose = m44(InverseBindPoseReals.data() + i);
            mInverseBindPoseMatrices.push_back(InverseBindPose);
        }

        for(i32 NodeIdx : tinyModel->scenes[0].nodes) {
            traverseNodes(tinyModel, NodeIdx, -1, m44(1.0f));
        }

        loadJointsFromNodes(tinyModel, Skin);
}

void
GLTFModel::loadJointsFromNodes(tinygltf::Model *tinyModel, const tinygltf::Skin &skin) {
    // NOTE(Jovan): Get all joints
    for(u32 i = 0; i < skin.joints.size(); ++i) {
        i32 SkinJointIdx = skin.joints[i];
        i32 NodeIdx = mNodeToNodeIdx[SkinJointIdx];
        const Node &N = mNodes[NodeIdx];
        mNodeToJointIdx[SkinJointIdx] = mJoints.size();

        Joint J;
        J.mParentIdx = -1;
        J.mIdx = SkinJointIdx;
        J.mName = N.mName;
        J.mLocalTransform = N.mLocalTransform;
        J.mInverseBindTransform = mInverseBindPoseMatrices[i];

        if(mNodeToJointIdx.find(N.mParentIdx) != mNodeToJointIdx.end()) {
            J.mParentIdx = N.mParentIdx;
        }

        mJoints.push_back(J);
    }
    mJointCount = mJoints.size();

    assert(mJoints.size() <= tinyModel->nodes.size());

    for(u32 i = 0; i < skin.joints.size(); ++i) {
        assert(mNodeToJointIdx[skin.joints[i]] == i);
    }
}

void
GLTFModel::loadAnimations(tinygltf::Model *tinyModel) {
    for(u32 AnimIdx = 0; AnimIdx < tinyModel->animations.size(); ++AnimIdx) {
        const tinygltf::Animation &TinyAnimation = tinyModel->animations[AnimIdx];
        Animation CurrAnim = Animation(AnimIdx);

        for(u32 i = 0; i < TinyAnimation.channels.size(); ++i) {
            const tinygltf::AnimationChannel &Channel = TinyAnimation.channels[i];
            std::vector<i32> &Channels = CurrAnim.mNodeToChannel[Channel.target_node];
            Channels.push_back(i);
        }

        for(const Joint &J : mJoints) {
            const std::map<i32, std::vector<i32>>::const_iterator ChannelIt = CurrAnim.mNodeToChannel.find(J.mIdx);
            if(ChannelIt != CurrAnim.mNodeToChannel.end()) {
                for(i32 ChannelIdx : ChannelIt->second) {
                    const tinygltf::AnimationChannel &Channel = TinyAnimation.channels[ChannelIdx];
                    const tinygltf::AnimationSampler &Sampler = TinyAnimation.samplers[Channel.sampler];
                    const tinygltf::Accessor &Input = tinyModel->accessors[Sampler.input];
                    const tinygltf::Accessor &Output = tinyModel->accessors[Sampler.output];
                    const std::vector<r32> Times = LoadFloats(tinyModel, Sampler.input);
                    const std::vector<r32> Values = LoadFloats(tinyModel, Sampler.output);
                    const std::string &Path = Channel.target_path;
                    std::map<i32, Keyframes>::iterator KeyframesIt = CurrAnim.mJointKeyframes.find(J.mIdx);

                    CurrAnim.mDurationInSeconds = std::max(CurrAnim.mDurationInSeconds, Input.maxValues[0]);

                    if(KeyframesIt == CurrAnim.mJointKeyframes.end()) {
                        CurrAnim.mJointKeyframes[J.mIdx] = Keyframes();
                        KeyframesIt = CurrAnim.mJointKeyframes.find(J.mIdx);
                    }

                    KeyframesIt->second.Load(Path, Output.count, Times.data(), Values.data());
                }                
            }
        }
        mAnimations.push_back(CurrAnim);
    }
    mActiveAnimation = tinyModel->animations.size() > 0 ? 0 : -1;
}

void
GLTFModel::CalculateJointTransforms(std::vector<m44> &jointTransforms, r64 timeInSeconds) {
    std::vector<m44> LocalTransforms(mJoints.size());
    std::vector<m44> GlobalJointTransforms(mJoints.size());
    jointTransforms.resize(mJoints.size());

    for(u32 i = 0; i < mJoints.size(); ++i) {
        const Joint &J = mJoints[i];
        if(mAnimations.size() > 0) {
            Animation &CurrAnim = mAnimations[mActiveAnimation];
            std::map<i32, Keyframes>::iterator KeyframesIt = CurrAnim.mJointKeyframes.find(J.mIdx);
            if(KeyframesIt != CurrAnim.mJointKeyframes.end()) {
                Keyframes K = KeyframesIt->second;
                m44 T(1.0);
                m44 R(1.0);
                m44 S(1.0);
                r64 clampedTimeInSeconds = fmod(timeInSeconds, CurrAnim.mDurationInSeconds);

                if(K.mTranslation.mCount > 0) {
                    T.Translate(K.mTranslation.Interpolate(clampedTimeInSeconds));
                }

                if(K.mRotation.mCount > 0) {
                    R.Rotate(K.mRotation.Interpolate(clampedTimeInSeconds));
                }

                if(K.mScale.mCount > 0) {
                    S.Scale(v3(&K.mScale.Interpolate(clampedTimeInSeconds)[0]));
                }

                LocalTransforms[i] = S * R * T;
                continue;
            }
        }

        LocalTransforms[i] = m44(&mJoints[i].mLocalTransform[0][0]);
    }

    GlobalJointTransforms[0] = m44(&LocalTransforms[0][0][0]);
    for(u32 i = 1; i < mJoints.size(); ++i) {
        u32 ParentIdx = mNodeToJointIdx[mJoints[i].mParentIdx];
        GlobalJointTransforms[i] =  m44(&LocalTransforms[i][0][0]) * GlobalJointTransforms[ParentIdx];
    }

    for(u32 i = 0; i < mJoints.size(); ++i) {
        const Joint &J = mJoints[i];
        jointTransforms[i] = J.mInverseBindTransform * GlobalJointTransforms[i];
        jointTransforms[i] = jointTransforms[i] * mInverseGlobalTransform;
    }
}

m44
GLTFModel::getLocalTransform(const tinygltf::Node &node) {
    v3 TranslationVec(0.0f);
    quat RotationQuat(1.0f, 0.0f, 0.0f, 0.0f);
    v3 ScaleVec(1.0f);
    m44 LocalTransform(1.0f);

    if(node.translation.size() > 0) {
        TranslationVec = v3(node.translation.data());
    }

    if(node.rotation.size() > 0) {
        memcpy(&RotationQuat, node.rotation.data(), 4 * sizeof(r32));
    }

    if(node.scale.size() > 0) {
        ScaleVec = v3(node.scale.data());
    }

    if(node.matrix.size() > 0) {
        LocalTransform = m44(node.matrix.data());
    } else {
        m44 T = m44(1.0f).Translate(TranslationVec);
        m44 R(RotationQuat);
        m44 S = m44(1.0f).Scale(ScaleVec);
        LocalTransform = S * R * T;
    }

    return LocalTransform;
}

void
GLTFModel::Draw(const ShaderProgram &program) {
    for(u32 i = 0; i < mMeshes.size(); ++i) {
        mMeshes[i].Draw(program);
    }
}

void
GLTFModel::loadMesh(tinygltf::Model *tinyModel, u32 meshIdx) {
    tinygltf::Mesh &TinyMesh = tinyModel->meshes[meshIdx];
    tinygltf::Primitive &Primitive0 = TinyMesh.primitives[0];
    tinygltf::Material &TinyMaterial = tinyModel->materials[Primitive0.material];
    auto &Attributes = Primitive0.attributes;
    u32 IndexAccessordIdx = Primitive0.indices;
    u32 PositionAccessorIdx = Attributes["POSITION"];
    u32 NormalAccessorIdx = Attributes["NORMAL"];
    u32 TexCoordsAccessorIdx = Attributes["TEXCOORD_0"];
    u32 JointsAccessorIdx = Attributes["JOINTS_0"];
    u32 WeightsAccessorIdx = Attributes["WEIGHTS_0"];


    std::vector<Mesh::Vertex> Vertices;
    std::vector<r32> Positions = LoadFloats(tinyModel, PositionAccessorIdx);
    std::vector<r32> Normals = LoadFloats(tinyModel, NormalAccessorIdx);
    std::vector<r32> TexCoords = LoadFloats(tinyModel, TexCoordsAccessorIdx);
    std::vector<u32> Indices = LoadIndices(tinyModel, IndexAccessordIdx);
    std::vector<u32> Joints = LoadIndices(tinyModel, JointsAccessorIdx);
    std::vector<r32> Weights = LoadFloats(tinyModel, WeightsAccessorIdx);

    for(u32 i = 0, j = 0; i < Positions.size(); i += 3, j += 4) {
        Vertices.push_back(Mesh::Vertex(&Positions[i], &Normals[i], &TexCoords[i], &Joints[j], &Weights[j]));
    }

    Mesh NewMesh(Vertices, Indices);
    tinygltf::TextureInfo &TexInfo = TinyMaterial.pbrMetallicRoughness.baseColorTexture;
    tinygltf::Texture &TinyTexture = tinyModel->textures[TexInfo.index];
    std::map<std::string, Texture>::const_iterator TexIt = mTextures.find(TinyTexture.name);

    if(TexIt == mTextures.end()) {
        tinygltf::Image &TinyImage = tinyModel->images[TinyTexture.source];
        Texture Tex(TinyImage.width, TinyImage.height, TinyImage.image.data());
        mTextures[TinyTexture.name] = Tex;
        NewMesh.mMaterial.mDiffuseId = Tex.mId;
    } else {
        NewMesh.mMaterial.mDiffuseId = TexIt->second.mId;
    }

    mMeshes.push_back(NewMesh);
}

void
GLTFModel::traverseNodes(tinygltf::Model *tinyModel, i32 nodeIdx, i32 parentIdx, const m44 &parentTransform) {
    const tinygltf::Node CurrNode = tinyModel->nodes[nodeIdx];
    Node N;
    N.mIdx = nodeIdx;
    N.mParentIdx = parentIdx;
    N.mLocalTransform = getLocalTransform(CurrNode);
    N.mGlobalTransform = N.mLocalTransform * parentTransform; //parentTransform * N.mLocalTransform; // T?
    if(!CurrNode.name.empty()) {
        N.mName = CurrNode.name;
    }
    mNodeToNodeIdx[nodeIdx] = mNodes.size();
    mNodes.push_back(N);

    if(CurrNode.mesh >= 0) {
        mInverseGlobalTransform = ~N.mGlobalTransform;
        loadMesh(tinyModel, CurrNode.mesh);
    }

    for(i32 ChildIdx : CurrNode.children) {
        N.mChildren.push_back(ChildIdx);
        traverseNodes(tinyModel, ChildIdx, nodeIdx, N.mGlobalTransform);
    }
}

std::vector<r32>
GLTFModel::LoadFloats(tinygltf::Model *tinyModel, i32 accessorIdx) {
    const tinygltf::Accessor &Accessor = tinyModel->accessors[accessorIdx];
    std::vector<r32> RetVal;
    u32 Offset = tinyModel->bufferViews[Accessor.bufferView].byteOffset + Accessor.byteOffset;
    u32 NumPerVert;
    switch(Accessor.type) {
        case TINYGLTF_TYPE_SCALAR: NumPerVert = 1; break;
        case TINYGLTF_TYPE_VEC2: NumPerVert = 2; break;
        case TINYGLTF_TYPE_VEC3: NumPerVert = 3; break;
        case TINYGLTF_TYPE_VEC4: NumPerVert = 4; break;
        case TINYGLTF_TYPE_MAT4: NumPerVert = 16; break;
        default: {
            std::cerr << "Unsupported type\n";
        }
    }
    
    size_t DataSize = Accessor.count * NumPerVert * sizeof(r32);
    RetVal.resize(DataSize);
    memcpy(RetVal.data(), mData.data() + Offset, DataSize);

    return RetVal;
}

std::vector<u32>
GLTFModel::LoadIndices(tinygltf::Model *tinyModel, i32 accessorIdx) {
    const tinygltf::Accessor &Accessor = tinyModel->accessors[accessorIdx];
    std::vector<u32> RetVal;
    u32 BufferViewIdx = Accessor.bufferView;
    u32 BufferViewOffset = tinyModel->bufferViews[BufferViewIdx].byteOffset;
    u32 Count = Accessor.count;
    u32 ComponentType = Accessor.componentType;
    u32 Type = Accessor.type;
    // NOTE(Jovan): If it's a scalar, have to convert to 1 (tinygltf has scalar as value 65)
    // else tinyglyf's defines match their component count, eg VEC2 == 2
    u32 ComponentsPerElement = Type == TINYGLTF_TYPE_SCALAR ? 1 : Type;
    u32 Offset = BufferViewOffset + Accessor.byteOffset;
    
    if(ComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        u32 *pValue = reinterpret_cast<u32*>(mData.data() + Offset);
        for(u32 i = 0; i < Accessor.count * ComponentsPerElement; ++i) {
            u32 Value = *pValue;
            RetVal.push_back(*pValue);
            ++pValue;
        }
    } else if (ComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT
        || ComponentType == TINYGLTF_COMPONENT_TYPE_SHORT) {
        // NOTE(Jovan): Unsigned or signed short
        u16 *pValue = reinterpret_cast<u16*>(mData.data() + Offset);
        for(u32 i = 0; i < Accessor.count * ComponentsPerElement; ++i) {
            u16 Value = *pValue;
            RetVal.push_back(*pValue);
            ++pValue;
        }
    } else {
        // NOTE(Jovan): Unsigned byte
        u8 *pValue = reinterpret_cast<u8*>(mData.data() + Offset);
        for(u32 i = 0; i < Accessor.count * ComponentsPerElement; ++i) {
            u8 Value = *pValue;
            RetVal.push_back(Value);
            ++pValue;
        }
    }

    return RetVal;
}
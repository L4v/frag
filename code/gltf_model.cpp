#include "gltf_model.hpp"

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

  // TODO(Jovan): Tidy up, parameterize and move out / (reuse / refactor)
  // existing
  glGenTextures(1, &mId);
  glBindTexture(GL_TEXTURE_2D, mId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
}

template <> GLTFModel::Keyframes<quat>::Keyframes() { mElementCount = 4; }

template <typename T> GLTFModel::Keyframes<T>::Keyframes() {
  mElementCount = 3;
}

template <typename T>
void GLTFModel::Keyframes<T>::Load(const r32 *timesData,
                                   const r32 *valuesData) {
  mValues.resize(mCount);
  mTimes.resize(mCount);
  memcpy(mTimes.data(), timesData, mCount * sizeof(r32));
  memcpy(mValues.data(), valuesData, mCount * mElementCount * sizeof(r32));
}

template <typename T>
r32 GLTFModel::Keyframes<T>::calculateInterpolationFactor(T &start, T &end,
                                                          r64 timeInSeconds) {
  u32 StartIdx = 0;
  u32 EndIdx = 0;

  for (u32 i = 0; i < mTimes.size() - 1; ++i) {
    if (mTimes[i + 1] > timeInSeconds) {
      break;
    }
    StartIdx = i;
  }

  EndIdx = StartIdx + 1;
  r32 T1 = mTimes[StartIdx];
  r32 T2 = mTimes[EndIdx];
  r32 Factor = (timeInSeconds - T1) / (T2 - T1);
  start = mValues[StartIdx];
  end = mValues[EndIdx];

  return Factor;
}

template <> quat GLTFModel::Keyframes<quat>::Interpolate(r64 timeInSeconds) {
  if (mTimes.size() == 1) {
    return mValues[0];
  }

  quat Start, End;
  r32 Factor = calculateInterpolationFactor(Start, End, timeInSeconds);
  return Slerp(Start, End, Factor).GetNormalized();
}

template <typename T>
T GLTFModel::Keyframes<T>::Interpolate(r64 timeInSeconds) {
  if (mTimes.size() == 1) {
    return mValues[0];
  }

  T Start, End;
  r32 Factor = calculateInterpolationFactor(Start, End, timeInSeconds);
  return Lerp(Start, End, Factor);
}

void GLTFModel::AnimKeyframes::Load(const std::string &path, u32 count,
                                    const r32 *timesData,
                                    const r32 *valuesData) {
  if (path == "translation") {
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
  mSpeed = 1.0f;
}

r64 GLTFModel::Animation::GetAnimationTime(r64 timeInSeconds) {
  return fmod(timeInSeconds * mSpeed, mDurationInSeconds);
}

GLTFModel::Node::Node(const tinygltf::Node &node, i32 nodeIdx, i32 parentIdx,
                      const m44 &localTransform, const m44 &parentTransform) {
  mIdx = nodeIdx;
  mParentIdx = parentIdx;
  mLocalTransform = localTransform;
  mGlobalTransform = mLocalTransform * parentTransform;
  if (!node.name.empty()) {
    mName = node.name;
  }
}

GLTFModel::Joint::Joint(const GLTFModel::Node &node, i32 skinJointIdx,
                        const m44 &inverseBindPoseTransform) {
  mParentIdx = -1;
  mIdx = skinJointIdx;
  mName = node.mName;
  mLocalTransform = node.mLocalTransform;
  mInverseBindPoseTransform = inverseBindPoseTransform;
}

GLTFModel::GLTFModel(const std::string &filePath) {
  mModelTransform.LoadIdentity();
  tinygltf::Model tinyModel;
  loadData(&tinyModel, filePath);
  loadNodes(&tinyModel);
  loadAnimations(&tinyModel);
}

void GLTFModel::loadData(tinygltf::Model *tinyModel,
                         const std::string &filePath) {
  mFilePath = filePath;
  std::string fileExtension =
      filePath.substr(filePath.find_last_of(".") + 1, filePath.length());
  tinygltf::TinyGLTF Loader;
  std::string Err;
  std::string Warn;
  bool Ret = false;

  Ret = fileExtension == "gltf"
            ? Loader.LoadASCIIFromFile(tinyModel, &Err, &Warn, filePath)
            : Loader.LoadBinaryFromFile(tinyModel, &Err, &Warn, filePath);

  if (!Err.empty()) {
    std::cerr << "Error: " << Err << std::endl;
    return;
  }

  if (!Warn.empty()) {
    std::cerr << "Warning: " << Warn << std::endl;
    return;
  }

  if (!Ret) {
    std::cerr << "Error: Failed to load data" << std::endl;
    return;
  }

  for (const auto &Buffer : tinyModel->buffers) {
    mData.insert(std::end(mData), std::begin(Buffer.data),
                 std::end(Buffer.data));
  }
}

void GLTFModel::loadNodes(tinygltf::Model *tinyModel) {
  const tinygltf::Skin &Skin = tinyModel->skins[0];
  std::vector<r32> InverseBindPoseReals;
  loadFloats(tinyModel, Skin.inverseBindMatrices, InverseBindPoseReals);

  for (u32 i = 0; i < InverseBindPoseReals.size(); i += 16) {
    m44 InverseBindPose = m44(InverseBindPoseReals.data() + i);
    mInverseBindPoseMatrices.push_back(InverseBindPose);
  }

  for (i32 NodeIdx : tinyModel->scenes[0].nodes) {
    traverseNodes(tinyModel, NodeIdx, -1, m44(1.0f));
  }

  loadJointsFromNodes(tinyModel, Skin);
}

void GLTFModel::traverseNodes(tinygltf::Model *tinyModel, i32 nodeIdx,
                              i32 parentIdx, const m44 &parentTransform) {
  const tinygltf::Node TinyNode = tinyModel->nodes[nodeIdx];
  Node N(TinyNode, nodeIdx, parentIdx, getLocalTransform(TinyNode),
         parentTransform);

  mNodeToNodeIdx[nodeIdx] = mNodes.size();
  mNodes.push_back(N);

  if (TinyNode.mesh >= 0) {
    mInverseGlobalTransform = ~N.mGlobalTransform;
    loadMesh(tinyModel, TinyNode.mesh);
  }

  for (i32 ChildIdx : TinyNode.children) {
    N.mChildren.push_back(ChildIdx);
    traverseNodes(tinyModel, ChildIdx, nodeIdx, N.mGlobalTransform);
  }
}

void GLTFModel::loadJointsFromNodes(tinygltf::Model *tinyModel,
                                    const tinygltf::Skin &skin) {
  for (u32 i = 0; i < skin.joints.size(); ++i) {
    i32 SkinJointIdx = skin.joints[i];
    i32 NodeIdx = mNodeToNodeIdx[SkinJointIdx];
    const Node &N = mNodes[NodeIdx];
    mNodeToJointIdx[SkinJointIdx] = mJoints.size();
    Joint J(N, SkinJointIdx, mInverseBindPoseMatrices[i]);

    if (mNodeToJointIdx.find(N.mParentIdx) != mNodeToJointIdx.end()) {
      J.mParentIdx = N.mParentIdx;
    }

    mJoints.push_back(J);
  }

  mJointCount = mJoints.size();

  assert(mJoints.size() <= tinyModel->nodes.size());
  for (u32 i = 0; i < skin.joints.size(); ++i) {
    assert(mNodeToJointIdx[skin.joints[i]] == i);
  }
}

void GLTFModel::loadAnimations(tinygltf::Model *tinyModel) {
  mActiveAnimation = 0;
  for (u32 AnimIdx = 0; AnimIdx < tinyModel->animations.size(); ++AnimIdx) {
    const tinygltf::Animation &TinyAnimation = tinyModel->animations[AnimIdx];
    Animation CurrAnim = Animation(AnimIdx);

    for (u32 i = 0; i < TinyAnimation.channels.size(); ++i) {
      const tinygltf::AnimationChannel &Channel = TinyAnimation.channels[i];
      std::vector<i32> &Channels = CurrAnim.mNodeToChannel[Channel.target_node];
      Channels.push_back(i);
    }

    for (const Joint &J : mJoints) {
      const std::map<i32, std::vector<i32>>::const_iterator ChannelIt =
          CurrAnim.mNodeToChannel.find(J.mIdx);
      if (ChannelIt != CurrAnim.mNodeToChannel.end()) {
        for (i32 ChannelIdx : ChannelIt->second) {
          const tinygltf::AnimationChannel &Channel =
              TinyAnimation.channels[ChannelIdx];
          const tinygltf::AnimationSampler &Sampler =
              TinyAnimation.samplers[Channel.sampler];
          const tinygltf::Accessor &Input = tinyModel->accessors[Sampler.input];
          const tinygltf::Accessor &Output =
              tinyModel->accessors[Sampler.output];
          std::vector<r32> Times;
          loadFloats(tinyModel, Sampler.input, Times);
          std::vector<r32> Values;
          loadFloats(tinyModel, Sampler.output, Values);
          const std::string &Path = Channel.target_path;
          std::map<i32, AnimKeyframes>::iterator KeyframesIt =
              CurrAnim.mJointKeyframes.find(J.mIdx);

          CurrAnim.mDurationInSeconds =
              std::max(CurrAnim.mDurationInSeconds, Input.maxValues[0]);

          if (KeyframesIt == CurrAnim.mJointKeyframes.end()) {
            CurrAnim.mJointKeyframes[J.mIdx] = AnimKeyframes();
            KeyframesIt = CurrAnim.mJointKeyframes.find(J.mIdx);
          }

          KeyframesIt->second.Load(Path, Output.count, Times.data(),
                                   Values.data());
        }
      }
    }
    mAnimations.push_back(CurrAnim);
  }
  mActiveAnimation = tinyModel->animations.empty() ? 0 : &mAnimations[0];
}

void GLTFModel::calculateJointTransforms(std::vector<m44> &jointTransforms,
                                         r64 timeInSeconds) {
  std::vector<m44> LocalTransforms(mJoints.size());
  std::vector<m44> GlobalJointTransforms(mJoints.size());
  jointTransforms.resize(mJoints.size());

  for (u32 i = 0; i < mJoints.size(); ++i) {
    const Joint &J = mJoints[i];
    if (!mAnimations.empty()) {
      std::map<i32, AnimKeyframes>::iterator KeyframesIt =
          mActiveAnimation->mJointKeyframes.find(J.mIdx);
      if (KeyframesIt != mActiveAnimation->mJointKeyframes.end()) {
        AnimKeyframes K = KeyframesIt->second;
        m44 T(1.0);
        m44 R(1.0);
        m44 S(1.0);
        r64 clampedTimeInSeconds =
            mActiveAnimation->GetAnimationTime(timeInSeconds);

        if (K.mTranslation.mCount > 0) {
          T.Translate(K.mTranslation.Interpolate(clampedTimeInSeconds));
        }

        if (K.mRotation.mCount > 0) {
          R.Rotate(K.mRotation.Interpolate(clampedTimeInSeconds));
        }

        if (K.mScale.mCount > 0) {
          S.Scale(v3(&K.mScale.Interpolate(clampedTimeInSeconds)[0]));
        }

        LocalTransforms[i] = S * R * T;
        continue;
      }
    }

    LocalTransforms[i] = m44(&mJoints[i].mLocalTransform[0][0]);
  }

  GlobalJointTransforms[0] = m44(&LocalTransforms[0][0][0]);
  for (u32 i = 1; i < mJoints.size(); ++i) {
    u32 ParentIdx = mNodeToJointIdx[mJoints[i].mParentIdx];
    GlobalJointTransforms[i] =
        m44(&LocalTransforms[i][0][0]) * GlobalJointTransforms[ParentIdx];
  }

  for (u32 i = 0; i < mJoints.size(); ++i) {
    const Joint &J = mJoints[i];
    jointTransforms[i] = J.mInverseBindPoseTransform * GlobalJointTransforms[i];
    jointTransforms[i] = jointTransforms[i] * mInverseGlobalTransform;
  }
}

m44 GLTFModel::getLocalTransform(const tinygltf::Node &node) {
  m44 LocalTransform(1.0f);
  if (node.matrix.size() > 0) {
    LocalTransform = m44(node.matrix.data());
  } else {
    v3 TranslationVec =
        node.translation.size() > 0 ? v3(node.translation.data()) : v3(0.0f);

    v3 ScaleVec = node.scale.size() > 0 ? v3(node.scale.data()) : v3(1.0f);

    quat RotationQuat(1.0f, 0.0f, 0.0f, 0.0f);
    if (node.rotation.size() > 0) {
      memcpy(&RotationQuat, node.rotation.data(), 4 * sizeof(r32));
    }
    m44 T = m44(1.0f).Translate(TranslationVec);
    m44 R(RotationQuat);
    m44 S = m44(1.0f).Scale(ScaleVec);
    LocalTransform = S * R * T;
  }

  return LocalTransform;
}

void GLTFModel::render(const Shader &program) {
  program.SetUniform4m("uModel", mModelTransform);
  for (u32 i = 0; i < mMeshes.size(); ++i) {
    mMeshes[i].render(program);
  }
}

void GLTFModel::loadMeshVertices(tinygltf::Model *tinyModel,
                                 std::map<std::string, int> &attributes,
                                 std::vector<Mesh::Vertex> &outVertices) {
  u32 PositionAccessorIdx = attributes["POSITION"];
  u32 NormalAccessorIdx = attributes["NORMAL"];
  u32 TexCoordsAccessorIdx = attributes["TEXCOORD_0"];
  u32 JointsAccessorIdx = attributes["JOINTS_0"];
  u32 WeightsAccessorIdx = attributes["WEIGHTS_0"];

  std::vector<r32> Positions;
  loadFloats(tinyModel, PositionAccessorIdx, Positions);
  std::vector<r32> Normals;
  loadFloats(tinyModel, NormalAccessorIdx, Normals);
  std::vector<r32> TexCoords;
  loadFloats(tinyModel, TexCoordsAccessorIdx, TexCoords);
  std::vector<u32> Joints;
  loadIndices(tinyModel, JointsAccessorIdx, Joints);
  std::vector<r32> Weights;
  loadFloats(tinyModel, WeightsAccessorIdx, Weights);

  for (u32 i = 0, j = 0, k = 0; i < Positions.size(); i += 3, j += 4, k += 2) {
    outVertices.push_back(Mesh::Vertex(&Positions[i], &Normals[i],
                                       &TexCoords[k], &Joints[j], &Weights[j]));
  }
}

void GLTFModel::loadMesh(tinygltf::Model *tinyModel, u32 meshIdx) {
  tinygltf::Mesh &TinyMesh = tinyModel->meshes[meshIdx];
  tinygltf::Primitive &Primitive0 = TinyMesh.primitives[0];

  auto &Attributes = Primitive0.attributes;
  std::vector<Mesh::Vertex> Vertices;
  loadMeshVertices(tinyModel, Attributes, Vertices);
  mVerticesCount = Vertices.size();

  u32 IndexAccessordIdx = Primitive0.indices;
  std::vector<u32> Indices;
  loadIndices(tinyModel, IndexAccessordIdx, Indices);

  Mesh NewMesh(Vertices, Indices);

  tinygltf::Material &TinyMaterial = tinyModel->materials[Primitive0.material];
  tinygltf::TextureInfo &TexInfo =
      TinyMaterial.pbrMetallicRoughness.baseColorTexture;
  tinygltf::Texture &TinyTexture = tinyModel->textures[TexInfo.index];
  std::map<std::string, Texture>::const_iterator TexIt =
      mTextures.find(TinyTexture.name);

  if (TexIt == mTextures.end()) {
    tinygltf::Image &TinyImage = tinyModel->images[TinyTexture.source];
    Texture Tex(TinyImage.width, TinyImage.height, TinyImage.image.data());
    mTextures[TinyTexture.name] = Tex;
    NewMesh.mMaterial.mDiffuseId = Tex.mId;
  } else {
    NewMesh.mMaterial.mDiffuseId = TexIt->second.mId;
  }

  mMeshes.push_back(NewMesh);
}

void GLTFModel::loadFloats(tinygltf::Model *tinyModel, i32 accessorIdx,
                           std::vector<r32> &out) {
  const tinygltf::Accessor &Accessor = tinyModel->accessors[accessorIdx];
  u32 Offset = tinyModel->bufferViews[Accessor.bufferView].byteOffset +
               Accessor.byteOffset;
  u32 NumPerVert;
  switch (Accessor.type) {
  case TINYGLTF_TYPE_SCALAR:
    NumPerVert = 1;
    break;
  case TINYGLTF_TYPE_VEC2:
    NumPerVert = 2;
    break;
  case TINYGLTF_TYPE_VEC3:
    NumPerVert = 3;
    break;
  case TINYGLTF_TYPE_VEC4:
    NumPerVert = 4;
    break;
  case TINYGLTF_TYPE_MAT4:
    NumPerVert = 16;
    break;
  default: {
    std::cerr << "Unsupported type\n";
  }
  }

  size_t DataSize = Accessor.count * NumPerVert * sizeof(r32);
  out.resize(DataSize);
  memcpy(out.data(), mData.data() + Offset, DataSize);
}

void GLTFModel::loadIndices(tinygltf::Model *tinyModel, i32 accessorIdx,
                            std::vector<u32> &out) {
  const tinygltf::Accessor &Accessor = tinyModel->accessors[accessorIdx];
  u32 BufferViewIdx = Accessor.bufferView;
  u32 BufferViewOffset = tinyModel->bufferViews[BufferViewIdx].byteOffset;
  u32 Count = Accessor.count;
  u32 ComponentType = Accessor.componentType;
  u32 Type = Accessor.type;
  // NOTE(Jovan): If it's a scalar, have to convert to 1 (tinygltf has scalar as
  // value 65) else tinyglyf's defines match their component count, eg VEC2 == 2
  u32 ComponentsPerElement = Type == TINYGLTF_TYPE_SCALAR ? 1 : Type;
  u32 Offset = BufferViewOffset + Accessor.byteOffset;

  switch (ComponentType) {
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
    u32 *pValue = reinterpret_cast<u32 *>(mData.data() + Offset);
    for (u32 i = 0; i < Accessor.count * ComponentsPerElement; ++i) {
      u32 Value = *pValue;
      out.push_back(*pValue);
      ++pValue;
    }
  } break;
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
  case TINYGLTF_COMPONENT_TYPE_SHORT: {
    u16 *pValue = reinterpret_cast<u16 *>(mData.data() + Offset);
    for (u32 i = 0; i < Accessor.count * ComponentsPerElement; ++i) {
      u16 Value = *pValue;
      out.push_back(*pValue);
      ++pValue;
    }
  } break;
  default: {
    u8 *pValue = reinterpret_cast<u8 *>(mData.data() + Offset);
    for (u32 i = 0; i < Accessor.count * ComponentsPerElement; ++i) {
      u8 Value = *pValue;
      out.push_back(Value);
      ++pValue;
    }
  }
  }
}

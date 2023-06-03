#include "tiny_gltf_model_loader.hpp"

void TinyGltfModelLoader::load(GLTFModel &gltfModel) {
  tinygltf::Model tinyModel;
  loadData(tinyModel, gltfModel.mFilePath);
  loadNodes(&tinyModel, gltfModel);
  loadAnimations(&tinyModel, gltfModel);
}

void TinyGltfModelLoader::loadData(tinygltf::Model &tinyModel,
                                   const std::string &filePath) {
  std::string fileExtension =
      filePath.substr(filePath.find_last_of(".") + 1, filePath.length());
  tinygltf::TinyGLTF Loader;
  std::string Err;
  std::string Warn;
  bool Ret = false;

  Ret = fileExtension == "gltf"
            ? Loader.LoadASCIIFromFile(&tinyModel, &Err, &Warn, filePath)
            : Loader.LoadBinaryFromFile(&tinyModel, &Err, &Warn, filePath);

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

  for (const auto &Buffer : tinyModel.buffers) {
    mData.insert(std::end(mData), std::begin(Buffer.data),
                 std::end(Buffer.data));
  }
}

void TinyGltfModelLoader::loadNodes(tinygltf::Model *tinyModel,
                                    GLTFModel &gltfModel) {
  const tinygltf::Skin &Skin = tinyModel->skins[0];
  std::vector<r32> InverseBindPoseReals;
  loadFloats(tinyModel, Skin.inverseBindMatrices, InverseBindPoseReals);

  for (u32 i = 0; i < InverseBindPoseReals.size(); i += 16) {
    m44 InverseBindPose = m44(InverseBindPoseReals.data() + i);
    gltfModel.addInverseBindPoseMatrix(InverseBindPose);
  }

  for (i32 NodeIdx : tinyModel->scenes[0].nodes) {
    traverseNodes(tinyModel, gltfModel, NodeIdx, -1, m44(1.0f));
  }

  loadJointsFromNodes(tinyModel, gltfModel, Skin);
}

void TinyGltfModelLoader::loadJointsFromNodes(tinygltf::Model *tinyModel,
                                              GLTFModel &gltfModel,
                                              const tinygltf::Skin &skin) {
  for (u32 i = 0; i < skin.joints.size(); ++i) {
    i32 SkinJointIdx = skin.joints[i];
    // i32 NodeIdx = mNodeToNodeIdx[SkinJointIdx];
    i32 NodeIdx = gltfModel.getNodeIdxMappedToNode(SkinJointIdx);
    const Node N = gltfModel.getNode(NodeIdx);
    // mNodeToJointIdx[SkinJointIdx] = mJoints.size();
    gltfModel.mapNodeToJointIdx(
        SkinJointIdx,
        gltfModel
            .getJointCount()); // TODO(Jovan): Also move this to a function?
    Joint J(N, SkinJointIdx, gltfModel.getInverseBindPoseMatrix(i));

    // if (mNodeToJointIdx.find(N.mParentIdx) != mNodeToJointIdx.end()) {
    if (gltfModel.checkIfNodeToJointIdxExists(N.mParentIdx)) {
      J.mParentIdx = N.mParentIdx;
    }

    gltfModel.addJoint(J);
  }

  // mJointCount = mJoints.size();

  assert(gltfModel.getJointCount() <= tinyModel->nodes.size());
  for (u32 i = 0; i < skin.joints.size(); ++i) {
    // assert(mNodeToJointIdx[skin.joints[i]] == i);
    assert(gltfModel.getNodeIdxMappedToJoint(skin.joints[i]) == i);
  }
}

void TinyGltfModelLoader::traverseNodes(tinygltf::Model *tinyModel,
                                        GLTFModel &gltfModel, i32 nodeIdx,
                                        i32 parentIdx,
                                        const m44 &parentTransform) {
  const tinygltf::Node TinyNode = tinyModel->nodes[nodeIdx];
  Node N(TinyNode.name, nodeIdx, parentIdx, getLocalTransform(TinyNode),
         parentTransform);

  gltfModel.mapNodeToNodeIdx(
      nodeIdx, gltfModel.getNodeCount()); // TODO(Jovan): mapNodeToNodeCount???
  gltfModel.addNode(N); // TODO(Jovan): Or just add login to addNode

  if (TinyNode.mesh >= 0) {
    gltfModel.setInverseGlobalTransform(~N.mGlobalTransform);
    loadMesh(tinyModel, gltfModel, TinyNode.mesh);
  }

  for (i32 ChildIdx : TinyNode.children) {
    N.mChildren.push_back(ChildIdx);
    traverseNodes(tinyModel, gltfModel, ChildIdx, nodeIdx, N.mGlobalTransform);
  }
}

m44 TinyGltfModelLoader::getLocalTransform(const tinygltf::Node &node) {
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

void TinyGltfModelLoader::loadMesh(tinygltf::Model *tinyModel,
                                   GLTFModel &gltfModel, u32 meshIdx) {
  tinygltf::Mesh &TinyMesh = tinyModel->meshes[meshIdx];
  tinygltf::Primitive &Primitive0 = TinyMesh.primitives[0];

  auto &Attributes = Primitive0.attributes;
  std::vector<Mesh::Vertex> Vertices;
  loadMeshVertices(tinyModel, Attributes, Vertices);
  // mVerticesCount = Vertices.size(); // TODO(Jovan): Should it be stored here?

  u32 IndexAccessordIdx = Primitive0.indices;
  std::vector<u32> Indices;
  loadIndices(tinyModel, IndexAccessordIdx, Indices);

  Mesh NewMesh(Vertices, Indices);

  tinygltf::Material &TinyMaterial = tinyModel->materials[Primitive0.material];
  tinygltf::TextureInfo &TexInfo =
      TinyMaterial.pbrMetallicRoughness.baseColorTexture;
  tinygltf::Texture &TinyTexture = tinyModel->textures[TexInfo.index];
  i32 texId = gltfModel.getTextureIdByName(TinyTexture.name);

  if (texId == -1) {
    tinygltf::Image &TinyImage = tinyModel->images[TinyTexture.source];
    ModelTexture Tex(TinyImage.width, TinyImage.height, TinyImage.image.data());

    // mTextures[TinyTexture.name] = Tex;
    gltfModel.mapNameToTexture(TinyTexture.name, Tex);
    NewMesh.mMaterial.mDiffuseId = Tex.mId;
  } else {
    NewMesh.mMaterial.mDiffuseId = texId;
  }

  gltfModel.addMesh(NewMesh);
}

void TinyGltfModelLoader::loadMeshVertices(
    tinygltf::Model *tinyModel, std::map<std::string, int> &attributes,
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

void TinyGltfModelLoader::loadAnimations(tinygltf::Model *tinyModel,
                                         GLTFModel &gltfModel) {
  for (u32 AnimIdx = 0; AnimIdx < tinyModel->animations.size(); ++AnimIdx) {
    const tinygltf::Animation &TinyAnimation = tinyModel->animations[AnimIdx];
    Animation CurrAnim = Animation(AnimIdx);

    for (u32 i = 0; i < TinyAnimation.channels.size(); ++i) {
      const tinygltf::AnimationChannel &Channel = TinyAnimation.channels[i];
      std::vector<i32> &Channels = CurrAnim.mNodeToChannel[Channel.target_node];
      Channels.push_back(i);
    }

    for (const Joint &J : gltfModel.getJoints()) {
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
    gltfModel.addAnimation(CurrAnim);
  }
  gltfModel.setActiveAnimation(0);
  // gltfModel.setActiveAnimation(tinyModel->animations.empty() ? 0
  //                                                            :
  //                                                            &mAnimations[0])
  //                                                            // TODO(Jovan):
  //                                                            Find a better
  //                                                            way
}

void TinyGltfModelLoader::loadFloats(tinygltf::Model *tinyModel,
                                     i32 accessorIdx, std::vector<r32> &out) {
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
  default:
    std::cerr << "Unsupported type\n";
  }

  size_t DataSize = Accessor.count * NumPerVert * sizeof(r32);
  out.resize(DataSize);
  memcpy(out.data(), mData.data() + Offset, DataSize);
}

void TinyGltfModelLoader::loadIndices(tinygltf::Model *tinyModel,
                                      i32 accessorIdx, std::vector<u32> &out) {
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

#ifndef GLTF_MODEL_HPP
#define GLTF_MODEL_HPP

#include "animation.hpp"
#include "animation_skeleton.hpp"
#include "math3d.hpp"
#include "mesh.hpp"
#include "shader.hpp"
#include "types.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <iterator>
#include <map>
#include <string>

struct ModelTexture { // TODO(Jovan): Move out? Or leave if gltf model becomes
                      // jsut model
  r32 mWidth;
  r32 mHeight;

  u32 mId;
  ModelTexture();
  ModelTexture(r32 width, r32 height, const u8 *data);
};

class GLTFModel {

  std::vector<m44> mInverseBindPoseMatrices;
  std::map<i32, i32> mNodeToJointIdx;
  std::map<i32, i32> mNodeToNodeIdx;
  m44 mInverseGlobalTransform;
  std::vector<Animation> mAnimations;
  std::vector<Joint> mJoints;
  std::vector<Mesh> mMeshes;
  std::vector<Node> mNodes;

  std::map<std::string, ModelTexture> mTextures;

public:
  std::string mFilePath;
  u32 mJointCount;
  u32 mVerticesCount;
  m44 mModelTransform;
  Animation *mActiveAnimation;

  GLTFModel(const std::string &filePath);

  void
  setActiveAnimation(i32 animationIdx); // TODO(Jovan): Temp way of doing it
  void addInverseBindPoseMatrix(const m44 &matrix);
  void setInverseGlobalTransform(const m44 &transform);
  void mapNodeToNodeIdx(i32 key, i32 value);
  u32 getNodeCount() const;
  void addNode(const Node &node);
  std::vector<Joint> getJoints() const;
  void addAnimation(const Animation &animation);
  i32 getNodeIdxMappedToNode(i32 nodeIdx);
  Node getNode(i32 nodeIdx) const;
  void mapNodeToJointIdx(i32 key, i32 value);
  u32 getJointCount() const;
  m44 getInverseBindPoseMatrix(u32 idx) const;
  bool checkIfNodeToJointIdxExists(u32 nodeIdx) const;
  void addJoint(const Joint &joint);
  i32 getNodeIdxMappedToJoint(i32 nodeIdx);

  void render(const Shader &program);
  void calculateJointTransforms(std::vector<m44> &jointTransforms,
                                r64 timeInSeconds);
  void addMesh(const Mesh &mesh);
  i32 getTextureIdByName(const std::string &name);
  void mapNameToTexture(const std::string &name, const ModelTexture &texture);
};

#endif

#ifndef MODEL_HPP
#define MODEL_HPP

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

class Model {

  std::vector<m44> mInverseBindPoseMatrices;
  m44 mInverseGlobalTransform;
  std::vector<Animation> mAnimations;
  std::vector<Mesh> mMeshes;

  std::map<std::string, ModelTexture> mTextures;

  Joint &getJointByName(const std::string &name);

public:
  std::vector<Joint> mJoints;
  std::vector<Node> mNodes;
  std::string mFilePath;
  u32 mJointCount;
  u32 mVerticesCount;
  m44 mModelTransform;
  Animation *mActiveAnimation;

  Model(const std::string &filePath);

  void
  setActiveAnimation(i32 animationIdx); // TODO(Jovan): Temp way of doing it
  Animation *getActiveAnimation() const;
  void addInverseBindPoseMatrix(const m44 &matrix);
  void setInverseGlobalTransform(const m44 &transform);
  u32 getNodeCount() const;
  void addNode(Node &node);
  std::vector<Joint> getJoints() const;
  Joint getJointById(i32 id);
  i32 getJointIdxByExternalId(i32 externalId);
  void addAnimation(const Animation &animation);
  Node getNodeByExternalId(i32 externalId);
  Node getNodeById(i32 id) const;
  u32 getJointCount() const;
  m44 getInverseBindPoseMatrix(u32 idx) const;
  void addJoint(Joint &joint, i32 nodeParentIdx);

  void render(const Shader &program, u32 highlightedId);
  void calculateJointTransforms(std::vector<m44> &jointTransforms,
                                r64 timeInSeconds);
  void addMesh(const Mesh &mesh);
  i32 getTextureIdByName(const std::string &name);
  void mapNameToTexture(const std::string &name, const ModelTexture &texture);
  std::string getMeshNameById(u32 meshId) const;
  void freezeJoint(const std::string &name, r64 timeInSeconds);
  void unfreezeJoints();
};

#endif

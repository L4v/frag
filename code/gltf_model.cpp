#include "gltf_model.hpp"
#include "imodel_loader.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/tiny_gltf.h"

ModelTexture::ModelTexture() {
  mWidth = 0.0f;
  mHeight = 0.0f;
}

ModelTexture::ModelTexture(r32 width, r32 height, const u8 *data) {
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

GLTFModel::GLTFModel(const std::string &filePath) {
  mFilePath = filePath;
  mModelTransform.LoadIdentity();
  // load(modelLoader); // TODO(Jovan): Remove dependency
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
          T.Translate(K.mTranslation.interpolate(clampedTimeInSeconds));
        }

        if (K.mRotation.mCount > 0) {
          R.Rotate(K.mRotation.interpolate(clampedTimeInSeconds));
        }

        if (K.mScale.mCount > 0) {
          S.Scale(v3(&K.mScale.interpolate(clampedTimeInSeconds)[0]));
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

void GLTFModel::render(const Shader &program) {
  program.SetUniform4m("uModel", mModelTransform);
  for (u32 i = 0; i < mMeshes.size(); ++i) {
    mMeshes[i].render(program);
  }
}

void GLTFModel::setActiveAnimation(i32 animationIdx) {
  mActiveAnimation = &mAnimations[animationIdx];
}

void GLTFModel::addInverseBindPoseMatrix(const m44 &matrix) {
  mInverseBindPoseMatrices.push_back(matrix);
}

void GLTFModel::setInverseGlobalTransform(const m44 &transform) {
  mInverseGlobalTransform = transform;
}

void GLTFModel::mapNodeToNodeIdx(i32 key, i32 value) {
  mNodeToNodeIdx[key] = value;
}

u32 GLTFModel::getNodeCount() const { return mNodes.size(); }

void GLTFModel::addNode(const Node &node) { mNodes.push_back(node); }

std::vector<Joint> GLTFModel::getJoints() const { return mJoints; }

void GLTFModel::addAnimation(const Animation &animation) {
  mAnimations.push_back(animation);
}

i32 GLTFModel::getNodeIdxMappedToNode(i32 nodeIdx) {
  return mNodeToNodeIdx[nodeIdx];
}

Node GLTFModel::getNode(i32 nodeIdx) const { return mNodes[nodeIdx]; }

void GLTFModel::mapNodeToJointIdx(i32 key, i32 value) {
  mNodeToJointIdx[key] = value;
}

u32 GLTFModel::getJointCount() const { return mJoints.size(); }

m44 GLTFModel::getInverseBindPoseMatrix(u32 idx) const {
  return mInverseBindPoseMatrices[idx];
}

bool GLTFModel::checkIfNodeToJointIdxExists(u32 nodeIdx) const {
  return mNodeToJointIdx.find(nodeIdx) != mNodeToJointIdx.end();
}

void GLTFModel::addJoint(const Joint &joint) { mJoints.push_back(joint); }

i32 GLTFModel::getNodeIdxMappedToJoint(i32 nodeIdx) {
  return mNodeToJointIdx[nodeIdx];
}

void GLTFModel::addMesh(const Mesh &mesh) { mMeshes.push_back(mesh); }

i32 GLTFModel::getTextureIdByName(const std::string &name) {
  std::map<std::string, ModelTexture>::const_iterator texIt =
      mTextures.find(name);
  if (texIt == mTextures.end()) {
    return -1;
  }
  return texIt->second.mId;
}

void GLTFModel::mapNameToTexture(const std::string &name,
                                 const ModelTexture &texture) {
  mTextures[name] = texture;
}

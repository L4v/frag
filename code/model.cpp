#include "model.hpp"

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

Model::Model(const std::string &filePath) {
  mFilePath = filePath;
  mModelTransform.LoadIdentity();
}

void Model::calculateJointTransforms(std::vector<m44> &jointTransforms,
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

void Model::render(const Shader &program, u32 highlightedId) {
  program.SetUniform4m("uModel", mModelTransform);
  for (u32 i = 0; i < mMeshes.size(); ++i) {
    Mesh &mesh = mMeshes[i];
    u32 id = i + 1;
    r32 r =
        (id & 0x000000FF) / 255.0f; // TODO(Jovan): Thids should be moved out
    r32 g = ((id & 0x0000FF00) >> 8) / 255.0f;
    r32 b = ((id & 0x00FF0000) >> 16) / 255.0f;
    program.SetUniform4f("uId", v4(r, g, b, 1.0f));
    program.SetUniform4m("uModel", mModelTransform);
    if (highlightedId == id) {
      program.SetUniform4f("uHighlightColor", v4(1.0f, 0.0f, 1.0f, 0.0f));
    } else {
      program.SetUniform4f("uHighlightColor", v4(1.0f));
    }
    mesh.render(program);
  }
}

void Model::setActiveAnimation(i32 animationIdx) {
  mActiveAnimation = &mAnimations[animationIdx];
}

void Model::addInverseBindPoseMatrix(const m44 &matrix) {
  mInverseBindPoseMatrices.push_back(matrix);
}

void Model::setInverseGlobalTransform(const m44 &transform) {
  mInverseGlobalTransform = transform;
}

void Model::mapNodeToNodeIdx(i32 key, i32 value) {
  mNodeToNodeIdx[key] = value;
}

u32 Model::getNodeCount() const { return mNodes.size(); }

void Model::addNode(const Node &node) { mNodes.push_back(node); }

std::vector<Joint> Model::getJoints() const { return mJoints; }

void Model::addAnimation(const Animation &animation) {
  mAnimations.push_back(animation);
}

i32 Model::getNodeIdxMappedToNode(i32 nodeIdx) {
  return mNodeToNodeIdx[nodeIdx];
}

Node Model::getNode(i32 nodeIdx) const { return mNodes[nodeIdx]; }

void Model::mapNodeToJointIdx(i32 key, i32 value) {
  mNodeToJointIdx[key] = value;
}

u32 Model::getJointCount() const { return mJoints.size(); }

m44 Model::getInverseBindPoseMatrix(u32 idx) const {
  return mInverseBindPoseMatrices[idx];
}

bool Model::checkIfNodeToJointIdxExists(u32 nodeIdx) const {
  return mNodeToJointIdx.find(nodeIdx) != mNodeToJointIdx.end();
}

void Model::addJoint(const Joint &joint) { mJoints.push_back(joint); }

i32 Model::getNodeIdxMappedToJoint(i32 nodeIdx) {
  return mNodeToJointIdx[nodeIdx];
}

void Model::addMesh(const Mesh &mesh) { mMeshes.push_back(mesh); }

i32 Model::getTextureIdByName(const std::string &name) {
  std::map<std::string, ModelTexture>::const_iterator texIt =
      mTextures.find(name);
  if (texIt == mTextures.end()) {
    return -1;
  }
  return texIt->second.mId;
}

void Model::mapNameToTexture(const std::string &name,
                             const ModelTexture &texture) {
  mTextures[name] = texture;
}

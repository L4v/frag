#ifndef ANIMATION_SKELETON_HPP
#define ANIMATION_SKELETON_HPP

#include "math3d.hpp"
#include <string>
#include <vector>

struct Node {
  Node(const std::string &nodeName, i32 externalid, i32 parentIdx,
       const m44 &localTransform, const m44 &parentTransform) {
    mExternalId = externalid;
    mParentIdx = parentIdx;
    mLocalTransform = localTransform;
    mGlobalTransform = mLocalTransform * parentTransform;
    if (!nodeName.empty()) {
      mName = nodeName;
    }
  }
  i32 mId;
  i32 mExternalId;
  i32 mParentIdx;
  std::string mName;
  std::vector<i32> mChildren;
  m44 mLocalTransform;
  m44 mGlobalTransform;
};

struct Joint {
  Joint(const Node &node, i32 externalId, const m44 &inverseBindPoseTransform) {
    mParentIdx = -1;
    mExternalId = externalId;
    mLocalTransform = node.mLocalTransform;
    mInverseBindPoseTransform = inverseBindPoseTransform;
    mName = node.mName;
    mFrozenTimeInSeconds = -1.0;
  }
  i32 mId;
  i32 mExternalId;
  i32 mParentIdx;
  r64 mFrozenTimeInSeconds;
  m44 mLocalTransform;
  m44 mInverseBindPoseTransform;
  std::string mName;
};

#endif

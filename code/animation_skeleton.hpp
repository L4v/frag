#ifndef ANIMATION_SKELETON_HPP
#define ANIMATION_SKELETON_HPP

#include "math3d.hpp"
#include <string>
#include <vector>

struct Node {
  Node(const std::string &nodeName, i32 nodeIdx, i32 parentIdx,
       const m44 &localTransform, const m44 &parentTransform) {
    mIdx = nodeIdx;
    mParentIdx = parentIdx;
    mLocalTransform = localTransform;
    mGlobalTransform = mLocalTransform * parentTransform;
    if (!nodeName.empty()) {
      mName = nodeName;
    }
  }
  i32 mIdx;
  i32 mParentIdx;
  std::string mName;
  std::vector<i32> mChildren;
  m44 mLocalTransform;
  m44 mGlobalTransform;
};

struct Joint {
  Joint(const Node &node, i32 skinJointIdx,
        const m44 &inverseBindPoseTransform) {
    mParentIdx = -1;
    mIdx = skinJointIdx;
    mName = node.mName;
    mLocalTransform = node.mLocalTransform;
    mInverseBindPoseTransform = inverseBindPoseTransform;
  }
  i32 mIdx;
  i32 mParentIdx;
  std::string mName;
  m44 mLocalTransform;
  m44 mInverseBindPoseTransform;
};

#endif

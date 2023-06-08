#ifndef ANIMATION_HPP
#define ANIMATION_HPP
#include "math3d.hpp"
#include <cstring>
#include <map>
#include <string>
#include <vector>

class QuatKeyframes {
private:
  r32 calculateInterpolationFactor(quat &start, quat &end, r64 timeInSeconds);

public:
  u32 mCount; // TODO(Jovan): Should it really be acessible like this?
  std::vector<r32> mTimes;
  std::vector<quat> mValues;

  void load(const r32 *timesData, const r32 *valuesData);
  quat interpolate(r64 timeInSeconds);
};

class V3Keyframes {
private:
  r32 calculateInterpolationFactor(v3 &start, v3 &end, r64 timeInSeconds);

public:
  u32 mCount; // TODO(Jovan): Should it really be acessible like this?
  std::vector<r32> mTimes;
  std::vector<v3> mValues;

  void load(const r32 *timesData, const r32 *valuesData);
  v3 interpolate(r64 timeInSeconds);
};

struct AnimKeyframes {
  V3Keyframes mTranslation;
  QuatKeyframes mRotation;
  V3Keyframes mScale;

  void Load(const std::string &path, u32 count, const r32 *timesData,
            const r32 *valuesData);
};

struct Animation {
  i32 mIdx;
  r64 mDurationInSeconds;
  r32 mSpeed;
  std::map<i32, std::vector<i32>> mNodeToChannel;
  std::map<i32, AnimKeyframes> mJointKeyframes; // TODO(Jovan): To class
  std::string mFrozenJointName;
  r64 mFrozenJointTime;

  Animation(i32 idx);
  r64 GetAnimationTime(r64 timeInSeconds);
};
#endif

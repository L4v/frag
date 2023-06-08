#include "animation.hpp"

void QuatKeyframes::load(const r32 *timesData, const r32 *valuesData) {
  mValues.resize(mCount);
  mTimes.resize(mCount);
  memcpy(mTimes.data(), timesData, mCount * sizeof(r32));
  memcpy(mValues.data(), valuesData, mCount * 4 * sizeof(r32));
}

quat QuatKeyframes::interpolate(r64 timeInSeconds) {
  if (mTimes.size() == 1) {
    return mValues[0];
  }

  quat Start, End;
  r32 Factor = calculateInterpolationFactor(Start, End, timeInSeconds);
  return Slerp(Start, End, Factor).GetNormalized();
}

r32 QuatKeyframes::calculateInterpolationFactor(quat &start, quat &end,
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

void V3Keyframes::load(const r32 *timesData, const r32 *valuesData) {
  mValues.resize(mCount);
  mTimes.resize(mCount);
  memcpy(mTimes.data(), timesData, mCount * sizeof(r32));
  memcpy(mValues.data(), valuesData, mCount * 3 * sizeof(r32));
}

r32 V3Keyframes::calculateInterpolationFactor(v3 &start, v3 &end,
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

v3 V3Keyframes::interpolate(r64 timeInSeconds) {
  if (mTimes.size() == 1) {
    return mValues[0];
  }

  v3 Start, End;
  r32 Factor = calculateInterpolationFactor(Start, End, timeInSeconds);
  return Lerp(Start, End, Factor);
}

void AnimKeyframes::Load(const std::string &path, u32 count,
                         const r32 *timesData, const r32 *valuesData) {
  if (path == "translation") {
    mTranslation.mCount = count;
    mTranslation.load(timesData, valuesData);
  } else if (path == "rotation") {
    mRotation.mCount = count;
    mRotation.load(timesData, valuesData);
  } else if (path == "scale") {
    mScale.mCount = count; // TODO(Jovan): Is this below wrong???
    mScale.load(timesData, valuesData);
  }
}

Animation::Animation(i32 idx) {
  mIdx = idx;
  mDurationInSeconds = 0.0f;
  mSpeed = 1.0f;
  mFrozenJointName = "";
  mFrozenJointTime = 0.0;
}

r64 Animation::GetAnimationTime(r64 timeInSeconds) {
  return fmod(timeInSeconds * mSpeed, mDurationInSeconds);
}

#include "frag.hpp"
#include "math3d.hpp"
#include "tiny_gltf_model_loader.hpp"

OrbitalCamera::OrbitalCamera(r32 fov, r32 distance, r32 rotateSpeed,
                             r32 zoomSpeed, const v3 &worldUp, const v3 &target)
    : mWorldUp(worldUp), mTarget(target), mLeftRight(-10.0f, 10.0f),
      mBottomTop(-10.0f, 10.0f), mNearFar(0.1f, 10.0f) {
  mFOV = fov;
  mRadius = distance;
  mRotateSpeed = rotateSpeed;
  mZoomSpeed = zoomSpeed;
  mWorldUp = worldUp;
  mTarget = target;
  mYaw = PI_HALF;
  mPitch = 0.0f;
  updateVectors();
}

void OrbitalCamera::Rotate(r32 dYaw, r32 dPitch, r32 dt) {
  dYaw *= mRotateSpeed * dt;
  dPitch *= mRotateSpeed * dt;

  mYaw -= dYaw;
  mPitch -= dPitch;
  if (mYaw > 2.0f * PI) {
    mYaw -= 2.0f * PI;
  }

  if (mYaw < 0.0f) {
    mYaw += 2.0f * PI;
  }

  if (mPitch > PI_HALF - 1e-4f) {
    mPitch = PI_HALF - 1e-4f;
  }
  if (mPitch < -PI_HALF + 1e-4f) {
    mPitch = -PI_HALF + 1e-4f;
  }
  updateVectors();
}

void OrbitalCamera::Rotate(v2 diffs, r32 dt) {
  diffs *= mRotateSpeed * dt;
  r32 dYaw = diffs[0];
  r32 dPitch = diffs[1];

  mYaw -= dYaw;
  mPitch -= dPitch;
  if (mYaw > 2.0f * PI) {
    mYaw -= 2.0f * PI;
  }

  if (mYaw < 0.0f) {
    mYaw += 2.0f * PI;
  }

  if (mPitch > PI_HALF - 1e-4f) {
    mPitch = PI_HALF - 1e-4f;
  }
  if (mPitch < -PI_HALF + 1e-4f) {
    mPitch = -PI_HALF + 1e-4f;
  }
  updateVectors();
}

void OrbitalCamera::Zoom(r32 dy, r32 dt) {
  dy *= mZoomSpeed * dt;
  mRadius -= dy;
  if (mRadius <= 0.5f) {
    mRadius = 0.5f;
  }

  updateVectors();
}

void OrbitalCamera::updateVectors() {
  // mPosition.X = mDistance * COS(mYaw) * COS(mPitch);
  // mPosition.Y = -mDistance * SIN(mPitch);
  // mPosition.Z = mDistance * SIN(mYaw) * COS(mPitch);
  mPosition = v3(mTarget.X + mRadius * COS(mYaw) * COS(mPitch),
                 mTarget.Y + mRadius * SIN(mPitch),
                 mTarget.Z + mRadius * COS(mPitch) * SIN(mYaw));

  mFront = (mTarget - mPosition).GetNormalized();
  mRight = (mFront ^ mWorldUp).GetNormalized();
  mUp = (mRight ^ mFront).GetNormalized();
}

Window::Window(u32 width, u32 height) : mSize(width, height) {
  ;
  mSceneWindowFocused = false;
}

State::State(OrbitalCamera *camera)
    : mWindow(800, 600), mFramebuffer(1920, 1080), mCamera(camera),
      mBonesModel("../res/backleg1rigging_separation.gltf") {
  mNewInput = &mInputBuffers[0];
  mOldInput = &mInputBuffers[1];
  mCamera = camera;
  mProjection = m44(1.0f);
  mDT = 0.0f;
  mImGUIInitialized = false;
  mIsAnimationPaused = false;

  TinyGltfModelLoader tinyGltfModelLoader;
  tinyGltfModelLoader.load(mBonesModel);
  mCurrModel = &mBonesModel;
}

Input &State::GetNewInput() { return *mNewInput; }

const Input &State::GetOldInput() { return *mOldInput; }

void State::BeginFrame() {
  *mNewInput = (Input){0};
  KeyboardController *NewKC = &mNewInput->mKeyboard;
  KeyboardController *OldKC = &mOldInput->mKeyboard;
  for (u32 i = 0; i < ArrayCount(NewKC->mButtons); ++i) {
    NewKC->mButtons[i].mEndedDown = OldKC->mButtons[i].mEndedDown;
  }

  MouseController *NewMC = &mNewInput->mMouse;
  MouseController *OldMC = &mOldInput->mMouse;
  for (u32 i = 0; i < ArrayCount(mNewInput->mMouse.mButtons); ++i) {
    NewMC->mButtons[i].mEndedDown = OldMC->mButtons[i].mEndedDown;
  }
  NewMC->mCursorPos = OldMC->mCursorPos;
}

void State::EndFrame() {
  Input *Tmp = mNewInput;
  mNewInput = mOldInput;
  mOldInput = Tmp;
}

void UpdateState(State *state) {
  const Input *CurrInput = &state->GetNewInput();
  const KeyboardController *NewKC = &CurrInput->mKeyboard;
  OrbitalCamera *Camera = state->mCamera;
  if (NewKC->mPauseAnimation.mEndedDown &&
      NewKC->mPauseAnimation.mHalfTransitionCount) {
    state->mIsAnimationPaused = !state->mIsAnimationPaused;
  }

  const MouseController *MC = &CurrInput->mMouse;
  if (MC->mLeft.mEndedDown && state->mWindow.mSceneWindowFocused) {
    Camera->Rotate(CurrInput->mMouse.mCursorDiff, state->mDT);
  }

  if (state->mWindow.mSceneWindowFocused) {
    Camera->Zoom(MC->mScrollOffset, state->mDT);
  }

  // TODO(Jovan): Avoid calculating every frame if possible
  state->mProjection = perspective(Camera->mFOV,
                                   state->mFramebuffer.mSize.X /
                                       (r32)state->mFramebuffer.mSize.Y,
                                   0.1f, 100.0f);
}

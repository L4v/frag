#ifndef FRAG_HPP
#define FRAG_HPP
#include "framebuffer_gl.hpp"
#include "gltf_model.hpp"
#include "math3d.hpp"
#include "types.hpp"
#include "util.hpp"

struct Window {
  v2 mSize;
  bool mSceneWindowFocused;

  Window(u32 width, u32 height);
};

struct ButtonState {
  bool mEndedDown;
  u32 mHalfTransitionCount;
};

struct MouseController {
  union {
    ButtonState mButtons[2];
    struct {
      ButtonState mLeft;
      ButtonState mRight;
    };
  };
  v2 mCursorPos = v2(0.0f, 0.0f);
  v2 mCursorDiff = v2(0.0f, 0.0f);
  r32 mScrollOffset;
};

struct KeyboardController {
  union {
    ButtonState mButtons[2];
    struct {
      ButtonState mChangeModel;
      ButtonState mQuit;
    };
  };
};

struct Input {
  bool mFirstMouse = true;
  KeyboardController mKeyboard;
  MouseController mMouse;
};

// TODO(Jovan): In separate file?
// TODO(Jovan): Panning?
class OrbitalCamera {
public:
  r32 mFOV;
  r32 mPitch;
  r32 mYaw;
  r32 mRotateSpeed;
  r32 mRadius;
  r32 mZoomSpeed;
  v3 mWorldUp;
  v3 mPosition;
  v3 mFront;
  v3 mUp;
  v3 mRight;
  v3 mTarget;
  v2 mLeftRight;
  v2 mBottomTop;
  v2 mNearFar;

  // NOTE(Jovan): Orbital camera constructor
  OrbitalCamera(r32 fov, r32 distance, r32 rotateSpeed = 1e-2f,
                r32 zoomSpeed = 1.0f, const v3 &worldUp = v3(0.0f, 1.0f, 0.0f),
                const v3 &target = v3(0.0f));
  void Rotate(r32 dYaw, r32 dPitch, r32 dt);
  void Rotate(v2 diffs, r32 dt);
  void Zoom(r32 dy, r32 dt);

private:
  void updateVectors();
};

class State {
private:
  Input mInputBuffers[2] = {0};
  Input *mNewInput;
  Input *mOldInput;
  GLTFModel mBonesModel;
  GLTFModel mMusclesModel;

public:
  FramebufferGL mFramebuffer;
  OrbitalCamera *mCamera;
  GLTFModel *mCurrModel;
  Window mWindow;
  m44 mProjection;
  r32 mDT;
  r32 mCurrentTimeInSeconds;
  bool mImGUIInitialized;
  bool mShowBones;

  State(OrbitalCamera *camera);

  void BeginFrame();
  void EndFrame();
  Input &GetNewInput();
  const Input &GetOldInput();
  void UpdateModel();
};

void UpdateAndRender(State *state);

#endif

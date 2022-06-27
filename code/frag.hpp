#ifndef FRAG_HPP
#define FRAG_HPP
#include "util.hpp"
#include "math3d.hpp"
#include "model.hpp"

struct Window {
    v2   mSize;
    bool mSceneWindowFocused;

    Window(i32 width, i32 height);
};

struct KeyboardController {
    union {
        bool mButtons[2];
        struct {
            bool mChangeModel;
            bool mQuit;
        };
    };
};

struct Input {
    KeyboardController mKeyboard;
};

// TODO(Jovan): In separate file?
// TODO(Jovan): Panning?
class Camera {
public:
    r32         mFOV;
    r32         mPitch;
    r32         mYaw;
    r32         mRotateSpeed;
    r32         mDistance;
    r32         mZoomSpeed;
    v3          mWorldUp;
    v3          mPosition;
    v3          mFront;
    v3          mUp;
    v3          mRight;
    v3          mTarget;

    // NOTE(Jovan): Orbital camera constructor
    Camera(r32 fov, r32 distance, r32 rotateSpeed = 1e-3f, r32 zoomSpeed = 2e-2f, const v3 &worldUp = v3(0.0f, 1.0f, 0.0f), const v3 &target = v3(0.0f));
    void Rotate(r32 dYaw, r32 dPitch, r32 dt);
    void Zoom(r32 dy, r32 dt);

private:
    void updateVectors();
};

class State {
private:
    Input  mInputs[2];
    Input *mInputBuffer[2];

public:
    GLTFModel *mCurrModel;
    m44        mProjection;
    v2         mCursorPos;
    v2         mFramebufferSize;
    bool       mFirstMouse;
    bool       mImGUIInitialized;
    Camera    *mCamera;
    Input     *mInput;
    Window    *mWindow;
    u32        mFBOTexture;
    r32        mDT;
    bool       mLeftMouse;
    bool       mShowBones;

    State(Window *window, Camera *camera);
    void BeginFrame();
    void EndFrame();
};

void RenderAndUpdate();
#endif

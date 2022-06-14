#ifndef FRAG_HPP
#define FRAG_HPP
#include "math3d.hpp"
#include "model.hpp"

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
    Camera(r32 fov, r32 distance, r32 rotateSpeed = 1e-3f, r32 zoomSpeed = 2e-2f, const v3 &worldUp = v3(0.0f, 1.0f, 0.0f), const v3 &target = v3(0.0f)) {
        mFOV = fov;
        mDistance = distance;
        mRotateSpeed = rotateSpeed;
        mZoomSpeed = zoomSpeed;
        mWorldUp = worldUp;
        mTarget = target;

        mYaw = PI_HALF;
        mPitch = 0.0f;
        _UpdateVectors();
    }

    void
    Rotate(r32 dYaw, r32 dPitch, r32 dt) {
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

        if(mPitch > PI_HALF - 1e-4f) {
            mPitch = PI_HALF - 1e-4f;
        }
        if(mPitch < -PI_HALF + 1e-4f) {
            mPitch = -PI_HALF + 1e-4f;
        }
        _UpdateVectors();
    }

    void
    Zoom(r32 dy, r32 dt) {
        dy *= mZoomSpeed * dt;
        mDistance -= dy;
        if (mDistance <= 0.5f) {
            mDistance = 0.5f;
        }

        _UpdateVectors();
    }

private:
    void
    _UpdateVectors() {
        mPosition.X = mDistance * cos(mYaw) * cos(mPitch);
        mPosition.Y = -mDistance * sin(mPitch);
        mPosition.Z = mDistance * sin(mYaw) * cos(mPitch);
        mFront = (mTarget - mPosition).GetNormalized();
        mRight = (mFront ^ mWorldUp).GetNormalized();
        mUp = (mRight ^ mFront).GetNormalized();
    }
};


struct EngineState {
    Camera    *mCamera;
    m44       mProjection;
    v2        mCursorPos;
    v2        mFramebufferSize;
    r32       mDT;
    u32       mFBOTexture;

    bool      mSceneWindowFocused;
    bool      mFirstMouse;
    bool      mLeftMouse;
    bool      mImGUIInitialized;

    bool      mShowBones;

    GLTFModel *mCurrModel;

    EngineState(Camera *camera) {
        mCamera = camera;
        mProjection = m44(1.0f);
        mCursorPos = v2(0.0f, 0.0f);
        mDT = 0.0f;
        mLeftMouse = false;
        mImGUIInitialized = false;
        mSceneWindowFocused = false;
        mFirstMouse = true;
        mFramebufferSize = v2(1920, 1080);
        mShowBones = true;
    }
};
#endif

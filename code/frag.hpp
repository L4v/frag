#ifndef FRAG_HPP
#define FRAG_HPP
#include <glm/glm.hpp>
#include "types.hpp"

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))
#define PI 3.14159265358979f
#define PI_HALF PI / 2.0f

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

    glm::vec3 mWorldUp;
    glm::vec3 mPosition;
    glm::vec3 mFront;
    glm::vec3 mUp;
    glm::vec3 mRight;
    glm::vec3 mTarget;

    // NOTE(Jovan): Orbital camera constructor
    Camera(r32 fov, r32 distance, r32 rotateSpeed = 1.0f, r32 zoomSpeed = 2.0f, glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3 target = glm::vec3(0.0f)) {
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
        mPosition.x = mDistance * cos(mYaw) * cos(mPitch);
        mPosition.y = -mDistance * sin(mPitch);
        mPosition.z = mDistance * sin(mYaw) * cos(mPitch);
        mFront = glm::normalize(mTarget - mPosition);
        mRight = glm::normalize(glm::cross(mFront, mWorldUp));
        mUp = glm::normalize(glm::cross(mRight, mFront));
    }
};


struct EngineState {
    Camera    *mCamera;
    glm::mat4 mProjection;
    glm::vec2 mCursorPos;
    glm::vec2 mFramebufferSize;
    r32       mDT;
    u32       mFBOTexture;

    bool      mSceneWindowFocused;
    bool      mFirstMouse;
    bool      mLeftMouse;
    bool      mImGUIInitialized;

    EngineState(Camera *camera) {
        mCamera = camera;
        mProjection = glm::mat4(1.0f);
        mCursorPos = glm::vec2(0.0f, 0.0f);
        mDT = 0.0f;
        mLeftMouse = false;
        mImGUIInitialized = false;
        mSceneWindowFocused = false;
        mFirstMouse = true;
        mFramebufferSize = glm::vec2(1920, 1080);
    }
};

#endif

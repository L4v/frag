#include "frag.hpp"

Camera::Camera(r32 fov, r32 distance, r32 rotateSpeed, r32 zoomSpeed, const v3 &worldUp, const v3 &target)  {
    mFOV = fov;
    mDistance = distance;
    mRotateSpeed = rotateSpeed;
    mZoomSpeed = zoomSpeed;
    mWorldUp = worldUp;
    mTarget = target;

    mYaw = PI_HALF;
    mPitch = 0.0f;
    updateVectors();
}

void
Camera::Rotate(r32 dYaw, r32 dPitch, r32 dt) {
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
    updateVectors();
}

void
Camera::Zoom(r32 dy, r32 dt)  {
    dy *= mZoomSpeed * dt;
    mDistance -= dy;
    if (mDistance <= 0.5f) {
        mDistance = 0.5f;
    }

    updateVectors();
}

void
Camera::updateVectors() {
    mPosition.X = mDistance * COS(mYaw) * COS(mPitch);
    mPosition.Y = -mDistance * SIN(mPitch);
    mPosition.Z = mDistance * SIN(mYaw) * COS(mPitch);
    mFront = (mTarget - mPosition).GetNormalized();
    mRight = (mFront ^ mWorldUp).GetNormalized();
    mUp = (mRight ^ mFront).GetNormalized();
}

Window::Window(i32 width, i32 height) {
    mSize = v2(width, height);
    mSceneWindowFocused = false;
}

State::State(Window *window, Camera *camera) {
    mWindow = window;
    mInputBuffer[0] = &mInputs[0];
    mInputBuffer[1] = &mInputs[1];
    mInput = mInputBuffer[0];
    mCamera = camera;
    mProjection = m44(1.0f);
    mCursorPos = v2(0.0f, 0.0f);
    mDT = 0.0f;
    mLeftMouse = false;
    mImGUIInitialized = false;
    mFirstMouse = true;
    mFramebufferSize = v2(1920, 1080);
    mShowBones = true;
}

void
State::BeginFrame() {
    mInput = mInputBuffer[0];
    for(u32 i = 0; i < ArrayCount(mInputBuffer[0]->mKeyboard.mButtons); ++i) {
        mInputBuffer[0]->mKeyboard.mButtons[i] = mInputBuffer[1]->mKeyboard.mButtons[i];
    }
}

void
State::EndFrame() {
    std::swap(mInputBuffer[0], mInputBuffer[1]);
}
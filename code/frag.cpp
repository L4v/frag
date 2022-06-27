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
Camera::Rotate(v2 diffs, r32 dt) {
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

Window::Window(u32 width, u32 height) : mSize(width, height) {;
    mSceneWindowFocused = false;
}

State::State(Camera *camera) 
: mWindow(800, 600),
 mFramebuffer(1920, 1080),
 mCamera(camera),
 mBonesModel("../res/backleg_bones.gltf"),
 mMusclesModel("../res/backleg_muscles.glb") {
    mNewInput = &mInputBuffers[0];
    mOldInput = &mInputBuffers[1];
    mCamera = camera;
    mProjection = m44(1.0f);
    mDT = 0.0f;
    mImGUIInitialized = false;
    mShowBones = true;
}

Input&
State::GetNewInput() {
    return *mNewInput;
}

const Input&
State::GetOldInput() {
    return *mOldInput;
}

void
State::BeginFrame() {
    *mNewInput = (Input){0};
    KeyboardController *NewKC = &mNewInput->mKeyboard;
    KeyboardController *OldKC = &mOldInput->mKeyboard;
    for(u32 i = 0; i < ArrayCount(NewKC->mButtons); ++i) {
        NewKC->mButtons[i].mEndedDown = OldKC->mButtons[i].mEndedDown;
    }

    MouseController *NewMC = &mNewInput->mMouse;
    MouseController *OldMC = &mOldInput->mMouse;
    for(u32 i = 0; i < ArrayCount(mNewInput->mMouse.mButtons); ++i) {
        NewMC->mButtons[i].mEndedDown = OldMC->mButtons[i].mEndedDown;
    }
    NewMC->mCursorPos = OldMC->mCursorPos;

}

void
State::EndFrame() {
    Input *Tmp = mNewInput;
    mNewInput = mOldInput;
    mOldInput = Tmp;
}

void
State::UpdateModel() {
    mCurrModel = mShowBones
        ? &mBonesModel
        : &mMusclesModel;
}

void
UpdateAndRender(State *state) {
    const Input *CurrInput = &state->GetNewInput();
    const KeyboardController *NewKC = &CurrInput->mKeyboard;
    if(NewKC->mChangeModel.mEndedDown && NewKC->mChangeModel.mHalfTransitionCount) {
        state->mShowBones = !state->mShowBones;
    }

    const MouseController *MC = &CurrInput->mMouse;
    if(MC->mLeft.mEndedDown && state->mWindow.mSceneWindowFocused) {
        state->mCamera->Rotate(CurrInput->mMouse.mCursorDiff, state->mDT);
    }

    if(state->mWindow.mSceneWindowFocused) {
        state->mCamera->Zoom(MC->mScrollOffset, state->mDT);
    }

    state->UpdateModel();
}
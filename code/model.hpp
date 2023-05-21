#ifndef MODEL_HPP
#define MODEL_HPP

#include "include/tiny_gltf.h"
#include "mesh.hpp"
#include "shader.hpp"
#include <iostream>
#include <string>

class GLTFModel {
    class Texture {
        r32 mWidth;
        r32 mHeight;

    public:
        u32 mId;
        Texture();
        Texture(r32 width, r32 height, const u8 *data);
    };

    template<typename T> class Keyframes {
    private:
        u8 mElementCount;

        r32 calculateInterpolationFactor(T &start, T &end, r64 timeInSeconds);
    public:
        Keyframes<T>();
        u32 mCount;
        std::vector<r32> mTimes;
        std::vector<T> mValues;

        void Load(const r32 *timesData, const r32 *valuesData);
        T Interpolate(r64 timeInSeconds);
    };

    struct AnimKeyframes {
        Keyframes<v3> mTranslation;
        Keyframes<quat> mRotation;
        Keyframes<v3> mScale;

        void Load(const std::string &path, u32 count, const r32 *timesData, const r32 *valuesData);
    };

    struct Animation {
        i32 mIdx;
        r64 mDurationInSeconds;
        r32 mSpeed;
        std::map<i32, std::vector<i32>> mNodeToChannel;
        std::map<i32, AnimKeyframes> mJointKeyframes;

        Animation(i32 idx);
        r64 GetAnimationTime(r64 timeInSeconds);
    };

    struct Node {
        Node(const tinygltf::Node& node, i32 nodeIdx, i32 parentIdx, const m44& localTransform, const m44& parentTransform);
        i32 mIdx;
        i32 mParentIdx;
        std::string mName;
        std::vector<i32> mChildren;
        m44 mLocalTransform;
        m44 mGlobalTransform;
    };

    struct Joint {
        Joint(const GLTFModel::Node& node, i32 skinJointIdx, const m44& inverseBindPoseTransform);
        i32 mIdx;
        i32 mParentIdx;
        std::string mName;
        m44 mLocalTransform;
        m44 mInverseBindPoseTransform;
    };

    std::vector<u8> mData;
    std::vector<m44> mInverseBindPoseMatrices;
    std::map<i32, i32> mNodeToJointIdx;
    std::map<i32, i32> mNodeToNodeIdx;
    m44 mInverseGlobalTransform;
    std::vector<Animation> mAnimations;
    std::vector<Joint> mJoints;
    std::vector<Mesh> mMeshes;
    std::vector<Node> mNodes;
    std::map<std::string, Texture> mTextures;

    void loadFloats(tinygltf::Model *tinyModel, i32 accessorIdx, std::vector<r32>& out);
    void loadIndices(tinygltf::Model *tinyModel, i32 accessorIdx, std::vector<u32>& out);
    m44 getLocalTransform(const tinygltf::Node &node);
    void loadData(tinygltf::Model *tinyModel, const std::string &filePath);
    void loadNodes(tinygltf::Model *tinyModel);
    void loadMeshVertices(tinygltf::Model* tinyModel, std::map<std::string, int>& attributes, std::vector<Mesh::Vertex>& outVertices);
    void loadMesh(tinygltf::Model *tinyModel, u32 meshIdx);
    void loadJointsFromNodes(tinygltf::Model *tinyModel, const tinygltf::Skin &skin);
    void loadAnimations(tinygltf::Model *tinyModel);
    void traverseNodes(tinygltf::Model *tinyModel, i32 nodeIdx, i32 parentIdx, const m44 &parentTransform);

public:
    std::string mFilePath;
    u32 mJointCount;
    u32 mVerticesCount;
    m44 mModelTransform;
    Animation *mActiveAnimation;

    GLTFModel(const std::string &filePath);
    void Render(const Shader &program);
    void CalculateJointTransforms(std::vector<m44> &jointTransforms, r64 timeInSeconds);
};

#endif
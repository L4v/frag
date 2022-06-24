#ifndef MESH_HPP
#define MESH_HPP

#include <vector>
#include "math3d.hpp"
#include "shader.hpp"

class Mesh {
public:
    struct Vertex {
        v3 mPosition;
        v2 mTexCoords;
        v3 mNormal;
        u32 mJointIds[4];
        v4 mWeights;

        Vertex(const r32 *position, const r32 *normal, const r32 *texCoords, const u32 *joints, const r32 *weights);
    };

    struct Material {
        i32 mDiffuseId = -1;
        i32 mSpecularId = -1;
    };

    Material mMaterial;

    Mesh(const std::vector<Vertex> &vertices, const std::vector<u32> &indices);
    void Render(const Shader &program);

private:
    std::vector<Vertex> mVertices;
    std::vector<u32> mIndices;
    u32 VAO;
    u32 VBO;
    u32 EBO;
};

#endif
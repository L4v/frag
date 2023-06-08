#ifndef MESH_HPP
#define MESH_HPP

#include "math3d.hpp"
#include "shader.hpp"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <vector>

class Mesh {
public:
  struct Vertex {
    v3 mPosition;
    v2 mTexCoords;
    v3 mNormal;
    u32 mJointIds[4];
    v4 mWeights;

    Vertex(const r32 *position, const r32 *normal, const r32 *texCoords,
           const u32 *joints, const r32 *weights);
  };

  struct Material {
    i32 mDiffuseId;
    i32 mSpecularId;
  };

  Material mMaterial;

  Mesh(const std::string &name, const std::vector<Vertex> &vertices,
       const std::vector<u32> &indices);
  void render(const Shader &program) const;
  std::string getName() const;
  // m44 &getModelTransform();
  // void setModelTransform(const m44 &transform); // TODO(Jovan): Needed?

private:
  std::vector<Vertex> mVertices;
  std::vector<u32> mIndices;
  u32 VAO;
  u32 VBO;
  u32 EBO;
  m44 mModelTransform;
  std::string mName;
};

#endif

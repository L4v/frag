#ifndef ASSIMP_MODEL_HPP
#define ASSIMP_MODEL_HPP

#include "math3d.hpp"
#include "mesh.hpp"
#include "shader.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <iostream>
#include <string>
#include <vector>

class AssimpModel {
private:
  std::vector<Mesh> mMeshes;

  void traverseNodes(const aiScene *scene, const aiNode *currNode);
  void loadMesh(const aiScene *scene, const aiMesh *mesh);
  void loadMeshVertices(const aiMesh *mesh,
                        std::vector<Mesh::Vertex> &outVertices);
  void loadMeshIndices(const aiMesh *mesh, std::vector<u32> &outIndices);

public:
  m44 mModelTransform;
  std::string mFilePath;
  u32 mVerticesCount;

  AssimpModel(const std::string &filePath);
  void render(const Shader &program) const;
};

#endif
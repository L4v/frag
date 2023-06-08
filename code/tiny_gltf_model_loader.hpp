#ifndef TINY_GLTF_MODEL_LOADER_HPP
#define TINY_GLTF_MODEL_LOADER_HPP

#include "animation_skeleton.hpp"
#include "imodel_loader.hpp"
#include "include/tiny_gltf.h"
#include "math3d.hpp"
#include "mesh.hpp"
#include "model.hpp"

class TinyGltfModelLoader : public IModelLoader {
private:
  std::vector<u8> mData;

  void loadFloats(tinygltf::Model *tinyModel, i32 accessorIdx,
                  std::vector<r32> &out);
  void loadIndices(tinygltf::Model *tinyModel, i32 accessorIdx,
                   std::vector<u32> &out);
  m44 getLocalTransform(const tinygltf::Node &node);
  void loadData(tinygltf::Model &tinyModel, const std::string &filePath);
  void loadNodes(tinygltf::Model *tinyModel, Model &gltfModel);
  void loadMeshVertices(tinygltf::Model *tinyModel,
                        std::map<std::string, int> &attributes,
                        std::vector<Mesh::Vertex> &outVertices);
  void loadMesh(tinygltf::Model *tinyModel, Model &gltfModel, u32 meshIdx,
                const std::string &meshName);
  void loadJointsFromNodes(tinygltf::Model *tinyModel, Model &gltfModel,
                           const tinygltf::Skin &skin);
  void loadAnimations(tinygltf::Model *tinyModel, Model &gltfModel);
  void traverseNodes(tinygltf::Model *tinyModel, Model &gltfModel, i32 nodeIdx,
                     i32 parentIdx, const m44 &parentTransform);

public:
  void load(Model &model);
};

#endif

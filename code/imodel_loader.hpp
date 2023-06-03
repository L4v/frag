#ifndef IMODEL_LOADER_HPP
#define IMODEL_LOADER_HPP

#include "gltf_model.hpp"
#include <iostream>
class IModelLoader {
public:
  virtual void load(GLTFModel &gltfModel) = 0;
};

#endif

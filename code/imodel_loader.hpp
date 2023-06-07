#ifndef IMODEL_LOADER_HPP
#define IMODEL_LOADER_HPP

#include "model.hpp"
#include <iostream>
class IModelLoader {
public:
  virtual void load(Model &gltfModel) = 0;
};

#endif

#include "assimp_model.hpp"

AssimpModel::AssimpModel(const std::string &filePath) {
  Assimp::Importer importer;

  const aiScene *scene =
      importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cerr << "Failed to import assimp model: " << importer.GetErrorString()
              << std::endl;
    return;
  }

  mVerticesCount = 0;
  mFilePath = filePath;
  mModelTransform.LoadIdentity();
}

void AssimpModel::traverseNodes(const aiScene *scene, const aiNode *currNode) {
  // TODO(Jovan): Load multiple meshes?
  if (currNode->mNumMeshes > 0) {
    const aiMesh *mesh = scene->mMeshes[currNode->mMeshes[0]];
    loadMesh(scene, mesh);
  }

  for (u32 i = 0; i < currNode->mNumChildren; ++i) {
    traverseNodes(scene, currNode->mChildren[i]);
  }
}

void AssimpModel::loadMesh(const aiScene *scene, const aiMesh *mesh) {
  std::vector<Mesh::Vertex> vertices;
  loadMeshVertices(mesh, vertices);
  mVerticesCount += vertices.size();

  std::vector<u32> indices;
  loadMeshIndices(mesh, indices);

  mMeshes.push_back(Mesh(mesh->mName.C_Str(), vertices, indices));
}

void AssimpModel::loadMeshVertices(const aiMesh *mesh,
                                   std::vector<Mesh::Vertex> &outVertices) {
  for (u32 vertexIdx = 0; vertexIdx < mesh->mNumVertices; ++vertexIdx) {
    r32 *position = &mesh->mVertices[vertexIdx].x;
    r32 *normal = &mesh->mNormals[vertexIdx].x;
    v3 zeroCoords(0.0f);
    r32 *texCoords = mesh->mTextureCoords[0]
                         ? &mesh->mTextureCoords[0][vertexIdx].x
                         : &zeroCoords[0];

    // TODO(JOVAN): HARDCODED
    r32 WEIGHTS[4] = {0.0f};
    u32 JOINTS[4] = {0};
    Mesh::Vertex vertex(position, normal, texCoords, JOINTS, WEIGHTS);
    outVertices.push_back(vertex);
  }
}

void AssimpModel::loadMeshIndices(const aiMesh *mesh,
                                  std::vector<u32> &outIndices) {
  for (u32 faceIdx = 0; faceIdx < mesh->mNumFaces; ++faceIdx) {
    const aiFace face = mesh->mFaces[faceIdx];
    for (u32 i = 0; i < face.mNumIndices; ++i) {
      outIndices.push_back(face.mIndices[i]);
    }
  }
}

void AssimpModel::render(const Shader &program) const {
  program.SetUniform4m("uModel", mModelTransform);
  for (const Mesh &mesh : mMeshes) {
    mesh.render(program);
  }
}

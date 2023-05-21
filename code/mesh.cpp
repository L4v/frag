#include "mesh.hpp"

Mesh::Vertex::Vertex(const r32 *position, const r32 *normal,
                     const r32 *texCoords, const u32 *joints,
                     const r32 *weights) {
  mPosition = v3(position);
  mNormal = v3(normal);
  mTexCoords = v2(texCoords);
  memcpy(mJointIds, joints, 4 * sizeof(u32));
  mWeights = v4(weights);
}

Mesh::Mesh(const std::vector<Vertex> &vertices,
           const std::vector<u32> &indices) {
  mVertices = std::vector<Vertex>(vertices);
  mIndices = std::vector<u32>(indices);

  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mVertices.size(),
               &mVertices[0], GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)(offsetof(Vertex, mTexCoords)));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)(offsetof(Vertex, mNormal)));
  glEnableVertexAttribArray(2);

  glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex),
                         (void *)(offsetof(Vertex, mJointIds)));
  glEnableVertexAttribArray(3);

  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)(offsetof(Vertex, mWeights)));
  glEnableVertexAttribArray(4);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mIndices[0]) * mIndices.size(),
               &mIndices[0], GL_STATIC_DRAW);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::render(const Shader &program) const {
  glUseProgram(program.mId);

  if (mMaterial.mDiffuseId >= 0) {
    glActiveTexture(GL_TEXTURE0);
    program.SetUniform1i("uDiffuse", 0);
    glBindTexture(GL_TEXTURE_2D, mMaterial.mDiffuseId);
  }

  glBindVertexArray(VAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glDrawElements(GL_TRIANGLES, mIndices.size(), GL_UNSIGNED_INT, 0);
}
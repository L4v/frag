#version 330 core

#define MAX_BONES 100

layout (location = 0) in vec3  aPos;
layout (location = 1) in vec2  aTexCoord;
layout (location = 2) in vec3  aNormal;
layout (location = 3) in ivec4 aBoneIds;
layout (location = 4) in vec4  aBoneWeights;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uBones[MAX_BONES];

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

out vec4 vertexColor;

void main() {
    mat4 BoneTransform = uBones[aBoneIds[0]] * aBoneWeights[0];
    BoneTransform     += uBones[aBoneIds[1]] * aBoneWeights[1];
    BoneTransform     += uBones[aBoneIds[2]] * aBoneWeights[2];
    BoneTransform     += uBones[aBoneIds[3]] * aBoneWeights[3];

    vec4 LocalPos = BoneTransform * vec4(aPos, 1.0f);

    gl_Position = uProjection * uView * uModel * LocalPos;
    TexCoord = aTexCoord;
    Normal = aNormal;
}

#version 330 core

layout (location = 0) in vec3  aPos;
layout (location = 1) in vec2  aTexCoord;
layout (location = 2) in vec3  aNormal;
layout (location = 3) in ivec4 aBoneIds;
layout (location = 4) in vec4  aBoneWeights;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;
flat out ivec4 BoneIds;
out vec4 BoneWeights;

void main() {
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    gl_Position = uProjection * uView * vec4(FragPos, 1.0);
    TexCoord = aTexCoord;
    Normal = aNormal;
    BoneIds = aBoneIds;
    BoneWeights = aBoneWeights;
}

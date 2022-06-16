#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

out vec4 vertexColor;

void main() {
    FragPos = vec3(vec4(aPos, 1.0) * uModel);
    gl_Position = vec4(FragPos, 1.0f) * uView * uProjection;
    TexCoord = aTexCoord;
    Normal = aNormal;
}

#version 330 core

out vec4 fragColor;
uniform vec4 uId;

void main() {
    fragColor = vec4(uId);
}

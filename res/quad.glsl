// #shader vertex
#version 330 core

layout (location = 0) in vec2 position;
out vec2 vUV;

void main() {
    vUV = position * 0.5 + 0.5;
    gl_Position = vec4(position, -1, 1);
}
// #shader fragment
#version 330 core
layout (location = 0) out vec4 glColor;
in vec2 vUV;
uniform sampler2D uTexture;

void main() {
    glColor = texture(uTexture, vUV);
}
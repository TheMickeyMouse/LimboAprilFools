// #shader vertex
#version 330
layout (location = 0) in vec2 pos;
out vec2 vUV;

void main() {
    vUV = pos;
    gl_Position = vec4(pos, 0, 1);
}

// #shader fragment
#version 330
layout (location = 0) out vec4 glColor;
uniform float innerRadius, outerRadius, alpha;
in vec2 vUV;

void main() {
    float dist = length(vUV);
    float x = (dist - innerRadius) / (outerRadius - innerRadius);
    glColor = mix(vec4(0), vec4(0, 0, 0, alpha), clamp(x, 0, 1));
}
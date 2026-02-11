// #shader vertex
#version 330 core
out vec2 vPosition;
void main() {
    gl_Position = vec4(vec2[3](vec2(-1,-1),vec2(3,-1),vec2(-1,3))[gl_VertexID], 0, 1);
    vPosition = gl_Position.xy * 0.5 + 0.5;
}
// #shader fragment
#version 330 core
layout (location = 0) out vec4 glColor;
in vec2 vPosition;

uniform sampler2D background, screenTex;
uniform float brightness, contrast;

vec4 over(vec4 a, vec4 b) { // a over b, a&b are premul'd
    return a + b * (1 - a.a);
}

void main() {
    vec3 result = over(texture(screenTex, vPosition), texture(background, vec2(vPosition.x, vPosition.y))).rgb;
    result += (3 + result.r + result.g + result.b) * contrast / 3;
    result = pow(result, vec3(1.0f / brightness));
//    result = (result - 0.5) * (1 + contrast) + 0.5;

    glColor = vec4(result, 1.0);
}
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
uniform float innerRadius, outerRadius;
uniform vec4 vignetteTint;
uniform vec2 aberrationOff;
uniform bool vignetteOver, spotlight;

vec4 over(vec4 a, vec4 b) { // a over b, a&b are premul'd
    return a + b * (1 - a.a);
}

void main() {
    vec2 uv = (vPosition * 2) - 1;
    vec4 R = texture(screenTex, vPosition + aberrationOff), B = texture(screenTex, vPosition - aberrationOff);
    vec4 abbColor = texture(screenTex, vPosition);
    abbColor.r = R.r;
    abbColor.b = B.b;
    // abbColor.a = (abbColor.a + R.a + B.a) / 3;

    float dist = (length(uv) - innerRadius) / (outerRadius - innerRadius);

    vec4 vignetteColor = vignetteTint * clamp(dist, 0, 1);
    vec4 result = vignetteOver ? over(vignetteColor, abbColor) : over(abbColor, vignetteColor);
    result.rgb = mix(texture(background, vPosition).rgb, result.rgb, result.a);
    result.a = 1;

    glColor = result;
}
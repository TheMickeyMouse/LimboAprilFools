#version 430 core

layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform readonly  image2D imgInput;
layout (rgba32f, binding = 1) uniform writeonly image2D imgOutput;
uniform float innerRadius, outerRadius;
uniform vec4 vignetteTint;
uniform ivec2 aberrationOff;

vec4 over(vec4 a, vec4 b) { // a over b
    float ba = b.a * (1 - a.a);
    return vec4(a.rgb * a.a + b.rgb * ba, a.a + ba);
}

void main() {
    ivec2 xy = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = (vec2(xy) / vec2(gl_NumWorkGroups.xy) * 2) - 1;
    vec4 R = imageLoad(imgInput, xy + aberrationOff), B = imageLoad(imgInput, xy - aberrationOff);
    vec4 abbColor = imageLoad(imgInput, xy);
    abbColor.r = R.r;
    abbColor.b = B.b;
    abbColor.a = (abbColor.a + R.a + B.a) / 3;

    float dist = (length(uv) - innerRadius) / (outerRadius - innerRadius);
    vec4 vignetteColor = mix(vec4(0), vignetteTint, clamp(dist, 0, 1));

    vec4 result = over(abbColor, vignetteColor);

    imageStore(imgOutput, xy, result.a == 0 ? vec4(0) : vec4(result.rgb / result.a, result.a));
}
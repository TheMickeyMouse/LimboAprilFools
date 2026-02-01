#version 430 core

layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform readonly  image2D imgInput;
layout (rgba32f, binding = 1) uniform writeonly image2D imgOutput;
layout (rgba8,   binding = 2) uniform readonly  image2D background;
uniform float innerRadius, outerRadius;
uniform vec4 vignetteTint;
uniform ivec2 aberrationOff;
uniform bool vignetteOver, spotlight;

vec4 mulalpha(vec4 col) {
    return vec4(col.rgb * col.a, col.a);
}

vec4 over(vec4 a, vec4 b) { // a over b, a&b are premul'd
    return a + b * (1 - a.a);
}

void main() {
    ivec2 xy = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = (vec2(xy) / vec2(gl_NumWorkGroups.xy) * 2) - 1;
    vec4 R = imageLoad(imgInput, xy + aberrationOff), B = imageLoad(imgInput, xy - aberrationOff);
    vec4 abbColor = imageLoad(imgInput, xy);
    abbColor.r = R.r;
    abbColor.b = B.b;
    // abbColor.a = (abbColor.a + R.a + B.a) / 3;

    float dist = (length(uv) - innerRadius) / (outerRadius - innerRadius);

    vec4 vignetteColor = vignetteTint * clamp(dist, 0, 1);
    vec4 result = vignetteOver ? over(vignetteColor, abbColor) : over(abbColor, vignetteColor);
    result.rgb = mix(imageLoad(background, xy).rgb, result.rgb, result.a);
    result.a = 1;

    imageStore(imgOutput, xy, result);
}
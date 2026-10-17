#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
// [region uniforms]
layout(std140, binding = 0) uniform Params {
    mat4 qt_Matrix;
    float qt_Opacity;
    float amount;
    float mode;
};
layout(binding = 1) uniform sampler2D source;
// [endregion]
// [region color]
void main() {
    vec4 pixel = texture(source, qt_TexCoord0);
    // Qt Quick的颜色已预乘alpha；先取非预乘颜色做灰度/反色，再乘回来。
    vec3 rgb = pixel.a > 0.00001 ? pixel.rgb / pixel.a : vec3(0.0);
    if (mode > 0.5 && mode < 1.5) {
        float gray = dot(rgb, vec3(0.299, 0.587, 0.114));
        rgb = mix(rgb, vec3(gray), amount);
    } else if (mode > 1.5) {
        rgb = mix(rgb, vec3(1.0) - rgb, amount);
    }
    fragColor = vec4(rgb * pixel.a, pixel.a) * qt_Opacity;
}
// [endregion]

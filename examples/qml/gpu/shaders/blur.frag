#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform Params {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 stepUV;
};
layout(binding = 1) uniform sampler2D source;
// [region blur]
void main() {
    // 一维五点二项式核；预乘颜色和alpha使用同一组权重。
    vec4 sum = texture(source, qt_TexCoord0 - 2.0 * stepUV);
    sum += 4.0 * texture(source, qt_TexCoord0 - stepUV);
    sum += 6.0 * texture(source, qt_TexCoord0);
    sum += 4.0 * texture(source, qt_TexCoord0 + stepUV);
    sum += texture(source, qt_TexCoord0 + 2.0 * stepUV);
    fragColor = sum * (qt_Opacity / 16.0);
}
// [endregion]

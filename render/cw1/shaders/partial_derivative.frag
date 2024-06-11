#version 450

layout(location = 0) in vec2 v2fTexCoord;
layout(location = 1) in vec3 v2fColor;

layout(set = 1, binding = 0) uniform sampler2D uTexColor;

layout(location = 0) out vec4 oColor;

void main()
{

    float partialX = abs(dFdx(gl_FragCoord.z));
    float partialY = abs(dFdy(gl_FragCoord.z));
    int scale = 10000;
    oColor = vec4(partialX*scale, partialY*scale, 0.0f, 1.0f);


}





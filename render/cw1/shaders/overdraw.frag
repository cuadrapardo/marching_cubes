#version 450

layout(location = 0) in vec2 v2fTexCoord;
layout(location = 1) in vec3 v2fColor;

layout(set = 1, binding = 0) uniform sampler2D uTexColor;

layout(location = 0) out vec4 oColor;



void main()
{
    oColor = vec4(texture(uTexColor, v2fTexCoord).rgb, 1.f );
    oColor = vec4(vec3(0.5,0,0),1.f) * oColor;
}
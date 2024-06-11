#version 450

layout(location = 0) in vec2 v2fTexCoord;
layout(location = 1) in vec3 v2fColor;


//layout( set = 0, binding = 0) uniform UScene{
//    mat4 camera;
//    mat4 projection;
//    mat4 projCam;
//} uScene;

layout(set = 1, binding = 0) uniform sampler2D uTexColor;

layout(location = 0) out vec4 oColor;

float LinearizeDepth(float depth){
//    float nearPlane = uScene.projection[3][2] / (uScene.projection[2][2] - 1.0f);
//    float farPlane = uScene.projection[3][2] / (uScene.projection[2][2] + 1.0f);
    float nearPlane =  0.1f;
    float farPlane = 100.f;
    float ndc = gl_FragCoord.z * 2.0 - 1.0;

    return (2.0 * nearPlane * farPlane) / (farPlane+nearPlane-ndc * (farPlane-nearPlane));
}

void main()
{

    float depthLinear = LinearizeDepth(gl_FragCoord.z)/100.f;
    oColor = vec4(vec3(depthLinear), 1.0f);


}





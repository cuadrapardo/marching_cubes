#version 450

//TODO: remove buffer for color. Unnecessary - use a push constant or similar to set color of all point cloud

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iColor;
layout(location = 2) in int iScalar;

layout( set = 0, binding = 0) uniform UScene{
    mat4 camera;
    mat4 projection;
    mat4 projCam;
} uScene;

layout(location = 0) out vec3 v2fColor;



void main() {
    gl_Position = uScene.projCam * vec4(iPosition, 1.0);
    gl_PointSize = iScalar; // point size

    v2fColor = iColor;
}

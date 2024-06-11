#version 450

layout(location = 0) in vec2 v2fTexCoord;
layout(location = 1) in vec3 v2fColor;

layout(set = 1, binding = 0) uniform sampler2D uTexColor;

layout(location = 0) out vec4 oColor;

const vec3 mipmapColors[16] = vec3[16](
    vec3( 0.97, 1.0f, 0.4f ),
    vec3( 0.02, 0.83, 0.62 ),
    vec3( 0.93, 0.27, 0.43 ),
    vec3( 0.38, 0.1f, 0.20 ),
    vec3( 0.1f, 0.6f, 0.66 ),
    vec3( 0.83, 0.62, 0.65 ),
    vec3( 1.0f, 0.76, 0.23 ),
    vec3( 0.5f, 0.0f, 0.0f ),
    vec3( 0.0f, 0.0f, 0.5f ),
    vec3( 0.5f, 0.5f, 0.0f ),
    vec3( 0.5f, 0.0f, 0.5f ),
    vec3( 0.2f, 0.7f, 0.4f ),
    vec3( 0.0f, 0.5f, 0.0f ),
    vec3( 0.0f, 0.5f, 0.5f ),
    vec3( 0.0f, 0.0f, 0.5f ),
    vec3( 0.4f, 0.7f, 0.4f )

);



void main()
{
    float lod = (textureQueryLod(uTexColor, v2fTexCoord)).x;
    float levelFloor = floor(lod);
    float levelCeil = ceil(lod);
    float t = fract(lod);

    vec3 colorFloor = mipmapColors[int(levelFloor)];
    vec3 colorCeil = mipmapColors[int(levelCeil)];

    // Perform blend between colorFloor and colorCeil depending on t
    vec3 finalColor = mix(colorFloor, colorCeil, smoothstep(0.0, 1.0, t));

    oColor = vec4(finalColor, 1);

}
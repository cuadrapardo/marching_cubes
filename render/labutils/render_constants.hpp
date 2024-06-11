//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_RENDER_CONSTANTS_HPP
#define MARCHING_CUBES_POINT_CLOUD_RENDER_CONSTANTS_HPP

#include <volk/volk.h>
#include "angle.hpp"
#include "error.hpp"

//Camera Settings:
constexpr float kCameraBaseSpeed = 1.7f; // units/second
constexpr float kCameraFastMult = 5.f; // speed multiplier
constexpr float kCameraSlowMult = 0.05f; // speed multiplier

constexpr float kCameraMouseSensitivity = 0.01f; // radians per pixel


namespace cfg
{
    // Compiled shader code for the graphics pipeline(s)
    // See sources in cw1/shaders/*.

#		define SHADERDIR_ "assets/cw1/shaders/"
    constexpr char const* kVertShaderPath = SHADERDIR_ "default.vert.spv";
    constexpr char const* kFragShaderPath = SHADERDIR_ "default.frag.spv";
#		undef SHADERDIR_


    // Models
#       define MODELDIR_ "assets/cw1/"
    constexpr char const* sponzaObj = MODELDIR_ "sponza_with_ship.obj";
#       undef MODELDIR_


    constexpr VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT;



}


#endif //MARCHING_CUBES_POINT_CLOUD_RENDER_CONSTANTS_HPP

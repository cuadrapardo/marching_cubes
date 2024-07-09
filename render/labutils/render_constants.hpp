//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_RENDER_CONSTANTS_HPP
#define MARCHING_CUBES_POINT_CLOUD_RENDER_CONSTANTS_HPP

#define OFF 0
#define ON 1

#define TEST_MODE OFF


#if TEST_MODE == ON
constexpr unsigned int TEST_CUBE_VERTEX_POSITIVE_VALUE = 2;
constexpr unsigned int TEST_CUBE_VERTEX_NEGATIVE_VALUE = 1;
// Maps vertex values (0- negative; 1 - positive) to a scalar value.
#define MAP_CLASSIFICATION_TO_VALUE(x) ((x) == 0 ? TEST_CUBE_VERTEX_NEGATIVE_VALUE : TEST_CUBE_VERTEX_POSITIVE_VALUE)
constexpr float TEST_ISOVALUE = 1.0f ;
#endif

#include <volk/volk.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "angle.hpp"
#include "error.hpp"
#include "vkbuffer.hpp"

using namespace labutils::literals;

//Camera Settings:
constexpr float kCameraBaseSpeed = 1.7f; // units/second
constexpr float kCameraFastMult = 5.f; // speed multiplier
constexpr float kCameraSlowMult = 0.05f; // speed multiplier

constexpr float kCameraMouseSensitivity = 0.01f; // radians per pixel


enum class EInputState
{
    forward,
    backward,
    strafeLeft,
    strafeRight,
    levitate,
    sink,
    fast,
    slow,
    mousing,
    max
};

struct UserState
{
    bool inputMap[std::size_t(EInputState::max)] = {};

    float mouseX = 0.f, mouseY = 0.f;
    float previousX = 0.f, previousY = 0.f;

    bool wasMousing = false;

    glm::mat4 camera2world = glm::identity<glm::mat4>();
};


namespace cfg
{
    // Compiled shader code for the graphics pipeline(s)
    // See sources in cw1/shaders/*.

#		define SHADERDIR_ "assets/cw1/shaders/"
    constexpr char const* kLineVertShaderPath = SHADERDIR_ "line.vert.spv";
    constexpr char const* kVertShaderPath = SHADERDIR_ "point.vert.spv";
    constexpr char const* kFragShaderPath = SHADERDIR_ "point.frag.spv";
    constexpr char const* kTriVertShaderPath = SHADERDIR_ "triangle.vert.spv";
    constexpr char const* kTriFragShaderPath = SHADERDIR_ "triangle.frag.spv";
#		undef SHADERDIR_


    // Models
#       define MODELDIR_ "assets/cw1/"
    constexpr char const* sponzaObj = MODELDIR_ "sponza_with_ship.obj";
    constexpr char const* torusTri = MODELDIR_ "cube.tri";
    constexpr char const* cubeOBJ = MODELDIR_ "cube.obj";

#       undef MODELDIR_

    constexpr char const* reconstructedOBJ = "reconstructed_surface.obj";


    constexpr VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT;

    // General rule: with a standard 24 bit or 32 bit float depth buffer,
    // you can support a 1:1000 ratio between the near and far plane with
    // minimal depth fighting. Larger ratios will introduce more depth
    // fighting problems; smaller ratios will increase the depth buffer's
    // resolution but will also limit the view distance
    constexpr float kCameraNear  = 0.1f;
    constexpr float kCameraFar   = 100.f;

    constexpr auto kCameraFov = 60.0_degf;

}

// Uniform data
namespace glsl
{
    struct SceneUniform{
        glm::mat4 camera;
        glm::mat4 projection;
        glm::mat4 projCam;
    };
    //vkCmdUpdateBuffer() has certain requirements, including the two below:
    static_assert(sizeof(SceneUniform) <= 65536, "SceneUniform must be less than 65536 bytes for vkCmdUpdateBuffer");

    static_assert(sizeof(SceneUniform) % 4 == 0,
                  "SceneUniform size must be a multiple of 4 bytes");

}


struct TexturedMesh{
    labutils::Buffer positions;
    labutils::Buffer texcoords;
    labutils::Buffer color;

    std::string diffuseTexturePath;

    std::uint32_t vertexCount;
};


#endif //MARCHING_CUBES_POINT_CLOUD_RENDER_CONSTANTS_HPP

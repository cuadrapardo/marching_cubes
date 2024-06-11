//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_CAMERA_HPP
#define MARCHING_CUBES_POINT_CLOUD_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

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


void update_user_state(UserState& user, float aElapsedTime);


#endif //MARCHING_CUBES_POINT_CLOUD_CAMERA_HPP

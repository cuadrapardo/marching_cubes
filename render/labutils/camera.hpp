//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_CAMERA_HPP
#define MARCHING_CUBES_POINT_CLOUD_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

#include "render_constants.hpp"

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

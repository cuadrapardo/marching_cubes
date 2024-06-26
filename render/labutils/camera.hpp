//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_CAMERA_HPP
#define MARCHING_CUBES_POINT_CLOUD_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

#include "render_constants.hpp"


void update_user_state(UserState& user, float aElapsedTime,  const bool& fly_cam, glm::vec3 const& modelCenter);


#endif //MARCHING_CUBES_POINT_CLOUD_CAMERA_HPP

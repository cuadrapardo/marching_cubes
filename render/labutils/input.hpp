//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#ifndef MARCHING_CUBES_POINT_CLOUD_INPUT_HPP
#define MARCHING_CUBES_POINT_CLOUD_INPUT_HPP

#include <GLFW/glfw3.h>


void glfw_callback_key_press( GLFWwindow*, int aKey, int aScanCode, int aAction, int aModifierFlags);
void glfw_callback_button( GLFWwindow* aWin, int aBut, int aAct, int );
void glfw_callback_motion(GLFWwindow* aWin, double aX, double aY );



#endif //MARCHING_CUBES_POINT_CLOUD_INPUT_HPP

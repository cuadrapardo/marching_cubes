//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#include "input.hpp"
#include "camera.hpp"
#include <assert.h>

void glfw_callback_key_press( GLFWwindow* aWindow, int aKey, int aScanCode, int aAction, int aModifierFlags )
{
    auto state = static_cast<UserState*>(glfwGetWindowUserPointer( aWindow ));
    assert( state );

    bool const isReleased = (GLFW_RELEASE == aAction);

    switch( aKey ) {
        case GLFW_KEY_W:
            state->inputMap[std::size_t(EInputState::forward)] = !isReleased;
            break;
        case GLFW_KEY_S:
            state->inputMap[std::size_t(EInputState::backward)] = !isReleased;
            break;
        case GLFW_KEY_A:
            state->inputMap[std::size_t(EInputState::strafeLeft)] = !isReleased;
            break;
        case GLFW_KEY_D:
            state->inputMap[std::size_t(EInputState::strafeRight)] = !isReleased;
            break;
        case GLFW_KEY_E:
            state->inputMap[std::size_t(EInputState::levitate)] = !isReleased;
            break;
        case GLFW_KEY_Q:
            state->inputMap[std::size_t(EInputState::sink)] = !isReleased;
            break;
        case GLFW_KEY_LEFT_SHIFT:
            [[fallthrough]];
        case GLFW_KEY_RIGHT_SHIFT:
            state->inputMap[std::size_t(EInputState::fast)] = !isReleased;
            break;
        case GLFW_KEY_LEFT_CONTROL:
            [[fallthrough]];
        case GLFW_KEY_RIGHT_CONTROL:
            state->inputMap[std::size_t(EInputState::slow)] = !isReleased;
            break;
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose( aWindow, GLFW_TRUE );
            break;
        default:
            ;
    }
}




void glfw_callback_button( GLFWwindow* aWin, int aBut, int aAct, int ){
    auto state = static_cast<UserState*>(glfwGetWindowUserPointer( aWin ));
    assert( state );

    if( GLFW_MOUSE_BUTTON_RIGHT == aBut && GLFW_PRESS == aAct ){
        auto& flag = state->inputMap[std::size_t(EInputState::mousing)];

        flag = !flag;
        if( flag )
            glfwSetInputMode( aWin, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
        else
            glfwSetInputMode( aWin, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
    }
}

void glfw_callback_motion( GLFWwindow* aWin, double aX, double aY ) {
    auto state = static_cast<UserState*>(glfwGetWindowUserPointer( aWin ));
    assert( state );
    state->mouseX = float(aX);
    state->mouseY = float(aY);
}


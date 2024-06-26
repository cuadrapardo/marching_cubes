//
// Created by Carolina Cuadra Pardo on 6/11/24.
//

#include "camera.hpp"


void update_user_state( UserState& aState, float aElapsedTime){ // glm::vec3 const& modelCenter
    auto& cam = aState.camera2world;

    if( aState.inputMap[std::size_t(EInputState::mousing)] )
    {
        // Only update the rotation on the second frame of mouse navigation. This ensures that the previousX
        // and Y variables are initialized to sensible values.
        if( aState.wasMousing )
        {
            auto const sens = kCameraMouseSensitivity;
            auto const dx = sens * (aState.mouseX-aState.previousX);
            auto const dy = sens * (aState.mouseY-aState.previousY);

            // Translate to the center of the model
//            cam = glm::translate(cam, modelCenter);

            cam = cam * glm::rotate( -dy, glm::vec3( 1.f, 0.f, 0.f ) );
            cam = cam * glm::rotate( -dx, glm::vec3( 0.f, 1.f, 0.f ) );

            // Translate back
//            cam = glm::translate(cam, -modelCenter);
        }

        aState.previousX = aState.mouseX;
        aState.previousY = aState.mouseY;
        aState.wasMousing = true;
    }
    else
    {
        aState.wasMousing = false;
    }

    auto const move = aElapsedTime * kCameraBaseSpeed *
                      (aState.inputMap[std::size_t(EInputState::fast)] ? kCameraFastMult:1.f) *
                      (aState.inputMap[std::size_t(EInputState::slow)] ? kCameraSlowMult:1.f);
    if( aState.inputMap[std::size_t(EInputState::forward)] )
        cam = cam * glm::translate( glm::vec3( 0.f, 0.f, -move ) );
    if( aState.inputMap[std::size_t(EInputState::backward)] )
        cam = cam * glm::translate( glm::vec3( 0.f, 0.f, +move ) );

    if( aState.inputMap[std::size_t(EInputState::strafeLeft)] )
        cam = cam * glm::translate( glm::vec3( -move, 0.f, 0.f ) );
    if( aState.inputMap[std::size_t(EInputState::strafeRight)] )
        cam = cam * glm::translate( glm::vec3( +move, 0.f, 0.f ) );

    if( aState.inputMap[std::size_t(EInputState::levitate)] )
        cam = cam * glm::translate( glm::vec3( 0.f, +move, 0.f ) );
    if( aState.inputMap[std::size_t(EInputState::sink)] )
        cam = cam * glm::translate( glm::vec3( 0.f, -move, 0.f ) );
}

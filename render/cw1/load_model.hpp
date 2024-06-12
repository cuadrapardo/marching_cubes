#ifndef LOAD_MODEL_OBJ_HPP_1B67CFB6_BF91_421E_983A_CA92A246F902
#define LOAD_MODEL_OBJ_HPP_1B67CFB6_BF91_421E_983A_CA92A246F902

#include "simple_model.hpp"
#include "../labutils/vkimage.hpp"
#include "../labutils/allocator.hpp"
#include "../labutils/vulkan_context.hpp"

constexpr char const* WHITE_MAT = "assets/cw1/textures/white.jpg";

// Load a Wavefront OBJ model
SimpleModel load_simple_wavefront_obj( char const* aPath );



#endif // LOAD_MODEL_OBJ_HPP_1B67CFB6_BF91_421E_983A_CA92A246F902


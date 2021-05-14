#pragma once
#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
constexpr auto COMPILED_SHADER_PATH = "Res\\CompiledShaders\\";
constexpr auto COMPILED_SHADER_SUFFIX = ".spv";
constexpr auto SHADER_PATH = "Res\\Shaders\\";
constexpr auto TEXTURE_PATH = "Res\\Textures\\";
constexpr auto MODELS_PATH = "Res\\Models\\";
#include <vector>
#include <GLM/glm.hpp>
//#include <stb/stb_image.h>


constexpr uint32_t MAX_OBJECTS = 10;

const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
#if VULKAN_SDK_INSTALLED
const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
#else
const std::vector<const char*> VALIDATION_LAYERS;
#endif

constexpr int MAX_FRAME_DRAWS = 2;

constexpr glm::vec3 GLOBAL_UP(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 GLOBAL_RIGHT(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 GLOBAL_FORWARD(0.0f, 0.0f, 1.0f);
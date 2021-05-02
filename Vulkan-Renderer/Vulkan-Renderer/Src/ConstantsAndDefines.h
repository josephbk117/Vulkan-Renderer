#pragma once
#include "Utils.h"
#define COMPILED_SHADER_PATH "Res\\CompiledShaders\\"
#define COMPILED_SHADER_SUFFIX ".spv"
#define SHADER_PATH = "Res\\Shaders\\"
#define PROFILE_SCOPE(name) BenchmarkTimer timer##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCSIG__)

constexpr uint32_t MAX_OBJECTS = 10;

const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

constexpr int MAX_FRAME_DRAWS = 2;

constexpr glm::vec3 GLOBAL_UP(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 GLOBAL_RIGHT(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 GLOBAL_FORWARD(0.0f, 0.0f, 1.0f);
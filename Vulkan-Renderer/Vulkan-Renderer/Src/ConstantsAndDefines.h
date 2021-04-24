#pragma once
#include "Utils.h"
#define COMPILED_SHADER_PATH "Res\\CompiledShaders\\"
#define COMPILED_SHADER_SUFFIX ".spv"
#define SHADER_PATH = "Res\\Shaders\\"
#define PROFILE_SCOPE(name) BenchmarkTimer timer##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCSIG__)

const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

constexpr int MAX_FRAME_DRAWS = 2;
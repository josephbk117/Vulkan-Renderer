#pragma once
#include "vulkan/vulkan_core.h"
#define COMPILED_SHADER_PATH "Res\\CompiledShaders\\"
#define COMPILED_SHADER_SUFFIX ".spv"
#define SHADER_PATH = "Res\\Shaders\\"

const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
#include "RenderPipeline.h"
#include "Utils.h"
#include <iostream>
#include "ConstantsAndDefines.h"

Renderer::RenderPipeline::RenderPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath)
{
	CreateGraphicsPipeline(COMPILED_SHADER_PATH + vertShaderPath + COMPILED_SHADER_SUFFIX, 
		COMPILED_SHADER_PATH + fragShaderPath + COMPILED_SHADER_SUFFIX);
}

void Renderer::RenderPipeline::CreateGraphicsPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath)
{
	using namespace Utilities;

	auto vertCode = Utils::ReadFile(vertShaderPath);
	auto fragCode = Utils::ReadFile(fragShaderPath);

	std::cout << "vertex shader code size " << vertCode.size() << std::endl;
	std::cout << "fragment shader code size " << fragCode.size() << std::endl;
}

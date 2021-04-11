#pragma once
#include <string>
namespace Renderer
{
	class RenderPipeline
	{
	public:
		RenderPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath);
	private:
		void CreateGraphicsPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath);
	};
}
#pragma once
#include <vector>
#include <string>
#include <fstream>

namespace Utilities
{
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;
		int presentationFamily = -1;

		bool IsValid()
		{
			return graphicsFamily >= 0 && presentationFamily >= 0;
		}
	};

	class Utils
	{
	public:

		static std::vector<char> ReadFile(const std::string& filePath)
		{
			std::ifstream file(filePath, std::ios::ate | std::ios::binary);
			if (!file.is_open())
			{
				throw std::runtime_error("failed to open file : " + filePath);
			}

			const size_t fileSize = static_cast<size_t>(file.tellg());
			std::vector<char> buffer(fileSize);

			file.seekg(0);
			file.read(buffer.data(), fileSize);

			file.close();
			return buffer;
		}
	};
}
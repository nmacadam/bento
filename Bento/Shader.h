#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>

#include <glslang/SPIRV/GlslangToSpv.h>

class Shader
{
public:
	Shader(vk::UniqueDevice & device, const char* path);
	~Shader();

	void create(vk::UniqueDevice & device, const char* path);

	/*bool GLSLtoSPV(const vk::ShaderStageFlagBits shaderType,
		std::string const &           glslShader,
		std::vector<unsigned int> &   spvShader);*/

	vk::UniqueShaderModule shaderModule;

private:
	vk::UniqueShaderModule createShaderModule(vk::UniqueDevice & device, const std::vector<char>& code);

	EShLanguage translateShaderStage(vk::ShaderStageFlagBits stage);
	static std::vector<char> readFile(const std::string& filename);
};


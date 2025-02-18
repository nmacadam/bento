#include "bpch.h"
#include "Shader.h"

#include <fstream>
#include <filesystem>

namespace bento
{
	Shader::Shader(vk::Device& device, const char* path)
	{
		create(device, path);
	}

	Shader::~Shader()
	{
	}

	void Shader::create(vk::Device& device, const char* path)
	{
		shaderModule = createShaderModule(device, readFile(path));
	}

	vk::UniqueShaderModule Shader::createShaderModule(vk::Device& device, const std::vector<char>& code)
	{
		return device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, code.size(), reinterpret_cast<const uint32_t*>(code.data())));
	}

	EShLanguage Shader::translateShaderStage(vk::ShaderStageFlagBits stage)
	{
		switch (stage)
		{
			case vk::ShaderStageFlagBits::eVertex: return EShLangVertex;
			case vk::ShaderStageFlagBits::eTessellationControl: return EShLangTessControl;
			case vk::ShaderStageFlagBits::eTessellationEvaluation: return EShLangTessEvaluation;
			case vk::ShaderStageFlagBits::eGeometry: return EShLangGeometry;
			case vk::ShaderStageFlagBits::eFragment: return EShLangFragment;
			case vk::ShaderStageFlagBits::eCompute: return EShLangCompute;
			case vk::ShaderStageFlagBits::eRaygenNV: return EShLangRayGenNV;
			case vk::ShaderStageFlagBits::eAnyHitNV: return EShLangAnyHitNV;
			case vk::ShaderStageFlagBits::eClosestHitNV: return EShLangClosestHitNV;
			case vk::ShaderStageFlagBits::eMissNV: return EShLangMissNV;
			case vk::ShaderStageFlagBits::eIntersectionNV: return EShLangIntersectNV;
			case vk::ShaderStageFlagBits::eCallableNV: return EShLangCallableNV;
			case vk::ShaderStageFlagBits::eTaskNV: return EShLangTaskNV;
			case vk::ShaderStageFlagBits::eMeshNV: return EShLangMeshNV;
			default: assert(false && "Unknown shader stage"); return EShLangVertex;
		}
	}

	std::vector<char> Shader::readFile(const std::string& filename)
	{
		/*if (!std::filesystem::exists(filename)) {
			throw std::runtime_error("file does not exist!");
		}*/

		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}
}

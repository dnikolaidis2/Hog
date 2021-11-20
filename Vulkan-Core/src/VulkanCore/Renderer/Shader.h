#pragma once
#include <shaderc/shaderc.h>
#include <vulkan/vulkan.h>

namespace VulkanCore {

	class Shader
	{
	public:
		Shader(const std::string& filepath);
		~Shader();

		VkShaderModule GetVertexShaderModule() { return m_VertexShaderModule; }
		VkShaderModule GetFragmentShaderModule() { return m_FragmentShaderModule; }
	private:
		std::string ReadFile(const std::string& filepath);
		std::unordered_map<shaderc_shader_kind, std::string> PreProcess(const std::string& source);

		void CompileOrGetVulkanBinaries(const std::unordered_map<shaderc_shader_kind, std::string>& shaderSources);

		void Reflect(shaderc_shader_kind stage, const std::vector<uint32_t>& shaderData);
		void CreateProgram();
		VkShaderModule CreateShaderModule(const std::vector<uint32_t>& code);
	private:
		VkDevice m_Device;

		std::string m_Name;
		std::string m_FilePath;

		std::unordered_map<shaderc_shader_kind, std::vector<uint32_t>> m_VulkanSPIRV;

		VkShaderModule m_VertexShaderModule;
		VkShaderModule m_FragmentShaderModule;
	};
}

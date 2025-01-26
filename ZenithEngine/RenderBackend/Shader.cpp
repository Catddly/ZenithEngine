#include "Shader.h"

#include "Core/Assertion.h"
#include "RenderDevice.h"
#include "VulkanHelper.h"

#include <fstream>
#include <vector>

namespace ZE::RenderBackend
{
	Shader::Shader(RenderDevice& renderDevice, const ByteCode& byteCode)
		: m_RenderDevice(renderDevice)
	{
		VulkanZeroStruct(VkShaderModuleCreateInfo, shaderCI);
		shaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCI.codeSize = byteCode.size();
		shaderCI.pCode = reinterpret_cast<const uint32_t*>(byteCode.data());
		
		VulkanCheckSucceed(vkCreateShaderModule(m_RenderDevice.m_Device, &shaderCI, nullptr, &m_Shader));
	}

	Shader::Shader(RenderDevice& renderDevice, const std::string& filepath)
		: m_RenderDevice(renderDevice)
	{
		std::ifstream inStream(filepath, std::ios::in | std::ios::binary);

		if (inStream.is_open())
		{
			std::string binary(std::istreambuf_iterator<char>(inStream), {});

			VulkanZeroStruct(VkShaderModuleCreateInfo, shaderCI);
			shaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderCI.codeSize = binary.size();
			shaderCI.pCode = reinterpret_cast<const uint32_t*>(binary.data());

			VulkanCheckSucceed(vkCreateShaderModule(m_RenderDevice.m_Device, &shaderCI, nullptr, &m_Shader));
		}

		inStream.close();
	}

	Shader::~Shader()
	{
		vkDestroyShaderModule(m_RenderDevice.m_Device, m_Shader, nullptr);
	}

	void Shader::ReleaseGPUShaderObject()
	{
		if (m_Shader)
		{
			vkDestroyShaderModule(m_RenderDevice.m_Device, m_Shader, nullptr);
			m_Shader = nullptr;
		}
	}

	Shader::LayoutBuilder& Shader::LayoutBuilder::PushResource(uint32_t set, EBindingResourceType type)
	{
		if (m_ResourceSetArray.size() < set + 1)
		{
			m_ResourceSetArray.resize(set + 1);
		}

		m_ResourceSetArray[set].emplace_back(type);

		return *this;
	}

	Shader::ShaderLayout Shader::LayoutBuilder::Build()
	{
		return ShaderLayout{ .m_ResourceSetArray = std::move(m_ResourceSetArray) };
	}

	VertexShader::InputLayoutBuilder& VertexShader::InputLayoutBuilder::AddLayout(VkFormat format, uint32_t size, uint32_t offset)
	{
		m_InputAttribArray.emplace_back(static_cast<uint32_t>(m_InputAttribArray.size()), 0, format, offset);
		m_TotalByteSize += size;

		return *this;
	}

	VertexShader::InputLayout VertexShader::InputLayoutBuilder::Build()
	{
		return InputLayout{ .m_InputAttribArray = std::move(m_InputAttribArray), .m_InputSizeInByte = m_TotalByteSize };
	}

	VertexShader::VertexShader(RenderDevice& renderDevice, const ByteCode& byteCode)
		: Shader(renderDevice, byteCode)
	{
	}

	VertexShader::VertexShader(RenderDevice& renderDevice, const std::string& filepath)
		: Shader(renderDevice, filepath)
	{
	}

	PixelShader::PixelShader(RenderDevice& renderDevice, const ByteCode& byteCode)
		: Shader(renderDevice, byteCode)
	{
	}

	PixelShader::PixelShader(RenderDevice& renderDevice, const std::string& filepath)
		: Shader(renderDevice, filepath)
	{
	}
}

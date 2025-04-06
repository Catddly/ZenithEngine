#include "Shader.h"

#include "Core/Assertion.h"
#include "Core/Hash.h"
#include "RenderDevice.h"
#include "VulkanHelper.h"

#include <fstream>
#include <vector>

namespace ZE::RenderBackend
{
	namespace
	{
		const char* ToStr(EShaderBindingResourceType type)
		{
			switch ( type )
			{
				case EShaderBindingResourceType::Unknown: return "Unknown";
				case EShaderBindingResourceType::UniformBuffer: return "UniformBuffer";
				case EShaderBindingResourceType::StorageBuffer: return "StorageBuffer";
				case EShaderBindingResourceType::Texture2D: return "Texture2D";
			}
			return "";
		}
	}
	
	Shader::Shader(RenderDevice& renderDevice, const ByteCode& byteCode)
	{
		SetRenderDevice(&renderDevice);
		
		VulkanZeroStruct(VkShaderModuleCreateInfo, shaderCI);
		shaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCI.codeSize = byteCode.size();
		shaderCI.pCode = reinterpret_cast<const uint32_t*>(byteCode.data());

		m_Hash = std::hash<ByteCode>{}(byteCode);
		
		VulkanCheckSucceed(vkCreateShaderModule(GetRenderDevice().GetNativeDevice(), &shaderCI, nullptr, &m_Shader));
	}

	Shader::Shader(RenderDevice& renderDevice, const std::string& filepath)
	{
		SetRenderDevice(&renderDevice);
		
		std::ifstream inStream(filepath, std::ios::in | std::ios::binary);

		if (inStream.is_open())
		{
			std::string binary(std::istreambuf_iterator<char>(inStream), {});

			VulkanZeroStruct(VkShaderModuleCreateInfo, shaderCI);
			shaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderCI.codeSize = binary.size();
			shaderCI.pCode = reinterpret_cast<const uint32_t*>(binary.data());

			m_Hash = std::hash<std::string>{}(binary);
			
			VulkanCheckSucceed(vkCreateShaderModule(GetRenderDevice().GetNativeDevice(), &shaderCI, nullptr, &m_Shader));
		}

		inStream.close();
	}

	Shader::~Shader()
	{
		if (m_Shader)
		{
			vkDestroyShaderModule(GetRenderDevice().GetNativeDevice(), m_Shader, nullptr);
			m_Shader = nullptr;
		}
	}

	void Shader::ReleaseGPUShaderObject()
	{
		if (m_Shader)
		{
			vkDestroyShaderModule(GetRenderDevice().GetNativeDevice(), m_Shader, nullptr);
			m_Shader = nullptr;
		}
	}

	Shader::LayoutBuilder::LayoutBuilder(Shader* pShader)
		: m_pShader(pShader)
	{}

	Shader::LayoutBuilder& Shader::LayoutBuilder::BindResource(uint32_t set, const std::string& name, EShaderBindingResourceType type)
	{
		if (m_ResourceSetBindings.size() < set + 1)
		{
			m_ResourceSetBindings.resize(set + 1);
		}
		
		if (auto iter = m_ResourceSetBindings[set].find(name); iter != m_ResourceSetBindings[set].end())
		{
			ZE_LOG_WARNING("Rebind resource {} in ... set ... from {} to {}", name.c_str(), ToStr(iter->second), ToStr(type));
			iter->second = type;
			return *this;
		}

		m_ResourceSetBindings[set].emplace(name, type);
		return *this;
	}

	void Shader::LayoutBuilder::Build()
	{
		m_pShader->m_Layout.m_ResourceSetArray = std::move(m_ResourceSetBindings);
	}

	VertexShader::InputLayoutBuilder::InputLayoutBuilder(VertexShader* pVertexShader)
		: m_pVertexShader(pVertexShader)
	{}

	VertexShader::InputLayoutBuilder& VertexShader::InputLayoutBuilder::AddLayout(VkFormat format, uint32_t size, uint32_t offset)
	{
		m_InputAttribArray.emplace_back(static_cast<uint32_t>(m_InputAttribArray.size()), 0, format, offset);
		m_TotalByteSize += size;

		return *this;
	}
	
	void VertexShader::InputLayoutBuilder::Build()
	{
		m_pVertexShader->m_InputLayout.m_InputAttribArray = std::move(m_InputAttribArray);
		m_pVertexShader->m_InputLayout.m_InputSizeInByte = m_TotalByteSize;

		// for (const auto& inputAttrib : m_pVertexShader->m_InputLayout.m_InputAttribArray)
		// {
		// 	m_pVertexShader->m_Hash = Core::Hash(m_pVertexShader->m_Hash, inputAttrib.location);
		// 	m_pVertexShader->m_Hash = Core::Hash(m_pVertexShader->m_Hash, inputAttrib.binding);
		// 	m_pVertexShader->m_Hash = Core::Hash(m_pVertexShader->m_Hash, inputAttrib.format);
		// 	m_pVertexShader->m_Hash = Core::Hash(m_pVertexShader->m_Hash, inputAttrib.offset);
		// }
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

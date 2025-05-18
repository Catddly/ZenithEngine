#include "Shader.h"

#include "Core/Hash.h"
#include "RenderBackend/RenderDevice.h"
#include "RenderBackend/VulkanHelper.h"

#include <fstream>
#include <vector>

namespace ZE::Render
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

	Shader* Shader::Create(RenderBackend::RenderDevice& renderDevice)
	{
		auto* pShader = new Shader;
		pShader->SetRenderDevice(&renderDevice);
		return pShader;
	}
	
	Shader::~Shader()
	{
		if (m_Shader)
		{
			vkDestroyShaderModule(GetRenderDevice().GetNativeDevice(), m_Shader, nullptr);
			m_Shader = nullptr;
		}
	}

	void Shader::ReleaseGPUShaderObject() const
	{
		if (m_Shader)
		{
			vkDestroyShaderModule(GetRenderDevice().GetNativeDevice(), m_Shader, nullptr);
			m_Shader = nullptr;
		}
	}

	Shader::LayoutBuilder::LayoutBuilder(const Shader* pShader)
		: m_Shader(pShader)
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
		m_Shader->m_Layout.m_ResourceSetArray = std::move(m_ResourceSetBindings);
	}

	VertexShader::InputLayoutBuilder::InputLayoutBuilder(const VertexShader* pVertexShader)
		: m_VertexShader(pVertexShader)
	{}

	VertexShader::InputLayoutBuilder& VertexShader::InputLayoutBuilder::AddLayout(VkFormat format, uint32_t size, uint32_t offset)
	{
		m_InputAttribArray.emplace_back(static_cast<uint32_t>(m_InputAttribArray.size()), 0, format, offset);
		m_TotalByteSize += size;

		return *this;
	}
	
	void VertexShader::InputLayoutBuilder::Build()
	{
		m_VertexShader->m_InputLayout.m_InputAttribArray = std::move(m_InputAttribArray);
		m_VertexShader->m_InputLayout.m_InputSizeInByte = m_TotalByteSize;

		// for (const auto& inputAttrib : m_pVertexShader->m_InputLayout.m_InputAttribArray)
		// {
		// 	m_pVertexShader->m_Hash = Core::Hash(m_pVertexShader->m_Hash, inputAttrib.location);
		// 	m_pVertexShader->m_Hash = Core::Hash(m_pVertexShader->m_Hash, inputAttrib.binding);
		// 	m_pVertexShader->m_Hash = Core::Hash(m_pVertexShader->m_Hash, inputAttrib.format);
		// 	m_pVertexShader->m_Hash = Core::Hash(m_pVertexShader->m_Hash, inputAttrib.offset);
		// }
	}
	
	VertexShader* VertexShader::Create(RenderBackend::RenderDevice& renderDevice)
	{
		auto* pShader = new VertexShader;
		pShader->SetRenderDevice(&renderDevice);
		return pShader;
	}

	PixelShader* PixelShader::Create(RenderBackend::RenderDevice& renderDevice)
	{
		auto* pShader = new PixelShader;
		pShader->SetRenderDevice(&renderDevice);
		return pShader;
	}
}

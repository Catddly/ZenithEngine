#pragma once

#include "Core/Reflection.h"
#include "RenderDeviceChild.h"

#include <refl.hpp>
#include <vulkan/vulkan_core.h>

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace ZE::RenderBackend
{
	class RenderDevice;

	enum class EShaderBindingResourceType : uint8_t
	{
		Unknown = 0,
		UniformBuffer,
		StorageBuffer,
		Texture2D,
	};
	
	class Shader : public Core::IReflectable, public RenderDeviceChild
	{
		ZE_CLASS_REFL()

	private:

		friend class PipelineState;
		friend class GraphicPipelineState;
		friend class GraphicPipelineStateBuilder;

		friend Core::ReflectImplFriend<Shader>;

	public:

		using ByteCode = std::vector<std::byte>;
		
		using ResourceNameToTypeMap = std::unordered_map<std::string, EShaderBindingResourceType>;

		struct Slot
		{
			EShaderBindingResourceType	m_ResourceType = EShaderBindingResourceType::Unknown;
		};

		struct Layout
		{
			std::vector<ResourceNameToTypeMap>			m_ResourceSetArray;
		};

		struct LayoutBuilder
		{
			LayoutBuilder(Shader* pShader);
			
			LayoutBuilder& BindResource(uint32_t set, const std::string& name, EShaderBindingResourceType type);

			void Build();

		private:

			Shader*										m_pShader = nullptr;
			std::vector<ResourceNameToTypeMap>			m_ResourceSetBindings;
		};

		Shader(RenderDevice& renderDevice, const ByteCode& byteCode);
		Shader(RenderDevice& renderDevice, const std::string& filepath);
		
		virtual ~Shader();

		virtual uint64_t GetHash() const { return m_Hash; }

	protected:

		void ReleaseGPUShaderObject();
		
		uint64_t							m_Hash = 0;
	
	private:

		Layout								m_Layout;
	
		VkShaderModule						m_Shader = nullptr;
	};

	class VertexShader final : public Shader
	{
		ZE_CLASS_REFL()

	private:

		friend class GraphicPipelineState;

		friend Core::ReflectImplFriend<VertexShader>;

	public:

		struct InputLayout
		{
			std::vector<VkVertexInputAttributeDescription>			m_InputAttribArray;
			uint32_t												m_InputSizeInByte = 0;
		};

		struct InputLayoutBuilder
		{
			InputLayoutBuilder(VertexShader* pVertexShader);
			
			InputLayoutBuilder& AddLayout(VkFormat format, uint32_t size, uint32_t offset);
			
			void Build();

		private:

			VertexShader*											m_pVertexShader = nullptr;
			uint32_t												m_TotalByteSize = 0;
			std::vector<VkVertexInputAttributeDescription>			m_InputAttribArray;
		};

		VertexShader(RenderDevice& renderDevice, const ByteCode& byteCode);
		VertexShader(RenderDevice& renderDevice, const std::string& filepath);
		virtual ~VertexShader() = default;

	private:

		InputLayout													m_InputLayout;
	};

	class PixelShader : public Shader
	{
		ZE_CLASS_REFL()

	private:

		friend class GraphicPipelineState;

		friend Core::ReflectImplFriend<PixelShader>;

	public:

		PixelShader(RenderDevice& renderDevice, const ByteCode& byteCode);
		PixelShader(RenderDevice& renderDevice, const std::string& filepath);
		virtual ~PixelShader() = default;

	private:

	};
}

REFL_AUTO(type(ZE::RenderBackend::Shader), field(m_Layout))
REFL_AUTO(type(ZE::RenderBackend::VertexShader, bases<ZE::RenderBackend::Shader>), field(m_InputLayout))
REFL_AUTO(type(ZE::RenderBackend::PixelShader, bases<ZE::RenderBackend::Shader>))

template <>
struct std::hash<ZE::RenderBackend::Shader::ByteCode>
{
	std::size_t operator()(const ZE::RenderBackend::Shader::ByteCode& value) const noexcept
	{
		return std::hash<std::string_view>{}(std::string_view{reinterpret_cast<const char*>(value.data()), value.size()});
	}
};

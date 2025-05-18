#pragma once

#include "Core/Reflection.h"
#include "Core/FileSystem.h"
#include "Asset/Asset.h"
#include "RenderBackend/RenderDeviceChild.h"

#include <refl.hpp>
#include <vulkan/vulkan_core.h>

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace ZE::RenderBackend
{
	class PipelineState;
	class GraphicPipelineState;
	class GraphicPipelineStateBuilder;
}

namespace ZE::Render
{
	class RenderDevice;
	class ShaderLoader;

	enum class EShaderBindingResourceType : uint8_t
	{
		Unknown = 0,
		UniformBuffer,
		StorageBuffer,
		Texture2D,
	};
	
	class Shader : public Asset::Asset, public RenderBackend::RenderDeviceChild
	{
		friend class ShaderLoader;
		
		ZE_CLASS_REFL(Shader)

	private:

		friend class RenderBackend::PipelineState;
		friend class RenderBackend::GraphicPipelineState;
		friend class RenderBackend::GraphicPipelineStateBuilder;

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
			LayoutBuilder(const Shader* pShader);
			
			LayoutBuilder& BindResource(uint32_t set, const std::string& name, EShaderBindingResourceType type);

			void Build();

		private:

			const Shader*								m_Shader = nullptr;
			std::vector<ResourceNameToTypeMap>			m_ResourceSetBindings;
		};
		
		Shader() = default;
		virtual ~Shader();

		static Shader* Create(RenderBackend::RenderDevice& renderDevice);
		
		virtual uint64_t GetHash() const { return m_Hash; }

		bool IsValid() const { return m_Shader != nullptr; }

	protected:

		void ReleaseGPUShaderObject() const;
		
		uint64_t							m_Hash = 0;

	private:

		// TODO: shader reflection, make InputLayout in load time
		mutable Layout						m_Layout;
	
		mutable VkShaderModule				m_Shader = nullptr;
	};

	class VertexShader final : public Shader
	{
		ZE_CLASS_REFL(VertexShader)

	private:

		friend class RenderBackend::GraphicPipelineState;

		friend Core::ReflectImplFriend<VertexShader>;

	public:

		struct InputLayout
		{
			std::vector<VkVertexInputAttributeDescription>			m_InputAttribArray;
			uint32_t												m_InputSizeInByte = 0;
		};

		struct InputLayoutBuilder
		{
			InputLayoutBuilder(const VertexShader* pVertexShader);
			
			InputLayoutBuilder& AddLayout(VkFormat format, uint32_t size, uint32_t offset);
			
			void Build();

		private:

			const VertexShader*										m_VertexShader = nullptr;
			uint32_t												m_TotalByteSize = 0;
			std::vector<VkVertexInputAttributeDescription>			m_InputAttribArray;
		};
		
		static VertexShader* Create(RenderBackend::RenderDevice& renderDevice);

	private:

		VertexShader() = default;

	private:

		// TODO: shader reflection, make InputLayout in load time
		mutable InputLayout											m_InputLayout;
	};

	class PixelShader : public Shader
	{
		ZE_CLASS_REFL(PixelShader)

	private:

		friend class RenderBackend::GraphicPipelineState;

		friend Core::ReflectImplFriend<PixelShader>;

	public:
		
		static PixelShader* Create(RenderBackend::RenderDevice& renderDevice);

	private:

		PixelShader() = default;
	};
}

REFL_AUTO(type(ZE::Render::Shader), field(m_Layout))
REFL_AUTO(type(ZE::Render::VertexShader, bases<ZE::Render::Shader>), field(m_InputLayout))
REFL_AUTO(type(ZE::Render::PixelShader, bases<ZE::Render::Shader>))

template <>
struct std::hash<ZE::Render::Shader::ByteCode>
{
	std::size_t operator()(const ZE::Render::Shader::ByteCode& value) const noexcept
	{
		return std::hash<std::string_view>{}(std::string_view{reinterpret_cast<const char*>(value.data()), value.size()});
	}
};

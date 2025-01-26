#pragma once

#include "Core/Reflection.h"

#include <refl.hpp>
#include <vulkan/vulkan_core.h>

#include <string>
#include <vector>
#include <limits>

namespace ZE::RenderBackend
{
	class RenderDevice;

	class Shader : public Core::IReflectable
	{
		ZE_CLASS_REFL()

	private:

		friend class PipelineState;
		friend class GraphicPipelineState;
		friend class GraphicPipelineStateBuilder;

		friend class ::refl_impl::metadata::type_info__<ZE::RenderBackend::Shader>;

	public:

		using ByteCode = std::vector<std::byte>;

		enum class EBindingResourceType
		{
			Unknown = 0,
			UniformBuffer,
			StorageBuffer,
			Texture2D,
		};

		struct Slot
		{
			//uint32_t				m_SetIndex = std::numeric_limits<uint32_t>::max();
			//uint32_t				m_BindingIndex = std::numeric_limits<uint32_t>::max();
			EBindingResourceType	m_ResourceType = EBindingResourceType::Unknown;
		};

		struct ShaderLayout
		{
			std::vector<std::vector<Slot>>					m_ResourceSetArray;
		};

		struct LayoutBuilder
		{
			LayoutBuilder& PushResource(uint32_t set, EBindingResourceType type);

			inline ShaderLayout Build();

		private:

			std::vector<std::vector<Slot>>					m_ResourceSetArray;
		};

		Shader(RenderDevice& renderDevice, const ByteCode& byteCode);
		Shader(RenderDevice& renderDevice, const std::string& filepath);
		virtual ~Shader();

	protected:

		void ReleaseGPUShaderObject();

	private:

		RenderDevice&						m_RenderDevice;

		ShaderLayout						m_Layout;
	
		VkShaderModule						m_Shader = nullptr;
	};

	class VertexShader final : public Shader
	{
		ZE_CLASS_REFL()

	private:

		friend class GraphicPipelineState;

		friend class ::refl_impl::metadata::type_info__<ZE::RenderBackend::VertexShader>;

	public:

		struct InputLayout
		{
			std::vector<VkVertexInputAttributeDescription>			m_InputAttribArray;
			uint32_t												m_InputSizeInByte = 0;
		};

		struct InputLayoutBuilder
		{
			InputLayoutBuilder& AddLayout(VkFormat format, uint32_t size, uint32_t offset);
			
			inline InputLayout Build();

		private:

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

		friend class ::refl_impl::metadata::type_info__<ZE::RenderBackend::PixelShader>;

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

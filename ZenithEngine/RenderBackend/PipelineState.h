#pragma once

#include "Shader.h"
#include "RenderDeviceChild.h"

#include <vulkan/vulkan_core.h>

#include <memory>
#include <array>
#include <vector>
#include <string>
#include <unordered_map>

namespace ZE::RenderBackend
{
	class RenderDevice;
	class Shader;
	class VertexShader;
	class PixelShader;

	enum class EPipelineStateType : uint8_t
	{
		Graphic = 0,
		Compute,
		Unknown
	};
	
	struct PipelineStateCreateDesc
	{
		// TODO: more precise and clean
		// [0] Vertex Shader
		// [1] Pixel  Shader
		std::array<std::shared_ptr<Shader>, 2>			m_Shaders = {};

		uint64_t GetHash() const;
	};

	class PipelineState : public RenderDeviceChild
	{
		friend class GraphicPipelineState;
		friend class RenderCommandList;
		friend class DescriptorCache;

	public:

		struct BoundShaderResourceLocation
		{
			static constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();
			
			[[nodiscard]] bool IsValid() const { return m_Set != kInvalidIndex && m_Binding != kInvalidIndex; }

			uint32_t GetSetIndex() const { return m_Set; }
			uint32_t GetBindingIndex() const { return m_Binding; }
			
		private:

			friend class PipelineState;

			BoundShaderResourceLocation() = default;
			BoundShaderResourceLocation(uint32_t set, uint32_t binding)
				: m_Set(set), m_Binding(binding)
			{}
			
			uint32_t			m_Set = kInvalidIndex;
			uint32_t			m_Binding = kInvalidIndex;
		};
		
	public:

		static bool CreateInPlace(PipelineState* pPipelineState, const PipelineStateCreateDesc& CreateDesc);

		BoundShaderResourceLocation FindBoundResourceLocation(const std::string& name);
		EShaderBindingResourceType FindBoundResourceType(const std::string& name);

		virtual EPipelineStateType GetPipelineType() const { return EPipelineStateType::Unknown; }
		
		virtual ~PipelineState();

	protected:

		VkPipelineLayout							m_Layout = nullptr;
		VkPipeline									m_Pipeline = nullptr;
		
		std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>	m_AllocatedSetBindingMap;
		std::unordered_map<std::string, EShaderBindingResourceType>		m_AllocatedResourceTypeMap;

	private:

		VkDescriptorPool							m_DescriptorPool = nullptr;
		std::vector<VkDescriptorSetLayout>			m_DescriptorSetLayouts;

	private:

		PipelineState(RenderDevice& renderDevice);
	};

	struct GraphicPipelineStateCreateDesc : public PipelineStateCreateDesc
	{
		std::vector<VkFormat>					m_ColorInputFormatArray;
		VkFormat								m_DepthStencilFormat = VK_FORMAT_UNDEFINED;

		void AddColorOutput(VkFormat format) { m_ColorInputFormatArray.emplace_back(format); }
		void SetDepthStencilOutput(VkFormat format) { m_DepthStencilFormat = format; }

		void SetVertexShader(std::shared_ptr<VertexShader> pVertexShader) { if (pVertexShader) { m_Shaders[0] = std::move(pVertexShader); } }
		void SetPixelShaderOptional(std::shared_ptr<PixelShader> pPixelShader) { if (pPixelShader) { m_Shaders[1] = std::move(pPixelShader); } }
	};
	
	class GraphicPipelineState final : public PipelineState
	{
	public:

		// struct Builder
		// {
		// 	Builder& BindVertexShader(const std::shared_ptr<VertexShader>& pVertexShader) { m_pVertexShader = pVertexShader; return *this; }
		// 	Builder& BindPixelShader(const std::shared_ptr<PixelShader>& pPixelShader) { m_pPixelShader = pPixelShader; return *this; }
		//
		// 	Builder& AddColorOutput(VkFormat format) { m_ColorInputFormatArray.emplace_back(format); return *this; }
		// 	Builder& SetDepthStencilOutput(VkFormat format) { m_DepthStencilFormat = format; return *this; }
		// 	
		// 	GraphicPipelineState* Build(RenderDevice& renderDevice);
		//
		// private:
		//
		// 	std::shared_ptr<VertexShader>			m_pVertexShader;
		// 	std::shared_ptr<PixelShader>			m_pPixelShader;
		//
		// 	std::vector<VkFormat>					m_ColorInputFormatArray;
		// 	VkFormat								m_DepthStencilFormat = VK_FORMAT_UNDEFINED;
		// };

		// struct Settings
		// {
		// 	std::vector<VkFormat>					m_ColorInputFormatArray;
		// 	VkFormat								m_DepthStencilFormat = VK_FORMAT_UNDEFINED;
		// };

		static GraphicPipelineState* Create(RenderDevice& renderDevice, const GraphicPipelineStateCreateDesc& CreateDesc);

		virtual EPipelineStateType GetPipelineType() const override { return EPipelineStateType::Graphic; }
		
		virtual ~GraphicPipelineState();

	private:

		GraphicPipelineState(RenderDevice& renderDevice);

	private:

		GraphicPipelineStateCreateDesc					m_CreateDesc;
	};
}

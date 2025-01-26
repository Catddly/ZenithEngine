#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

namespace ZE::RenderBackend
{
	class RenderDevice;
	class Shader;
	class VertexShader;
	class PixelShader;

	class PipelineState
	{
		friend class GraphicPipelineState;

	public:

		static bool CreateInPlace(PipelineState* pPipelineState);

		virtual ~PipelineState();

	protected:

		RenderDevice&								m_RenderDevice;

		VkPipelineLayout							m_Layout = nullptr;
		VkPipeline									m_Pipeline = nullptr;

		std::vector<std::shared_ptr<Shader>>		m_pShaderArray;

	private:

		VkDescriptorPool							m_DescriptorPool = nullptr;
		std::vector<VkDescriptorSetLayout>			m_DescriptorSetLayoutArray;

	private:

		PipelineState(RenderDevice& renderDevice);
	};

	class GraphicPipelineState final : public PipelineState
	{
	public:

		struct Builder
		{
			Builder& BindVertexShader(const std::shared_ptr<VertexShader>& pVertexShader) { m_pVertexShader = pVertexShader; }
			Builder& BindPixelShader(const std::shared_ptr<PixelShader>& pPixelShader) { m_pPixelShader = pPixelShader; }

			Builder& AddColorOutput(VkFormat format) { m_ColorInputFormatArray.emplace_back(format); }
			Builder& SetDepthStencilOutput(VkFormat format) { m_DepthStencilFormat = format; }
			
			GraphicPipelineState* Build(RenderDevice& renderDevice);

		private:

			std::shared_ptr<VertexShader>			m_pVertexShader;
			std::shared_ptr<PixelShader>			m_pPixelShader;
		
			std::vector<VkFormat>					m_ColorInputFormatArray;
			VkFormat								m_DepthStencilFormat = VK_FORMAT_UNDEFINED;
		};

		struct Settings
		{
			std::vector<VkFormat>					m_ColorInputFormatArray;
			VkFormat								m_DepthStencilFormat = VK_FORMAT_UNDEFINED;
		};

		static GraphicPipelineState* Create(RenderDevice& renderDevice, const Settings& settings, const std::shared_ptr<VertexShader>& pVertexShader, const std::shared_ptr<PixelShader>& pPixelShader = nullptr);

		virtual ~GraphicPipelineState();

	private:

		GraphicPipelineState(RenderDevice& renderDevice);
	};
}

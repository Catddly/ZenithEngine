#pragma once

#include "RenderDeviceChild.h"

#include <variant>
#include <glm/vec4.hpp>

#include <vulkan/vulkan_core.h>

namespace ZE::RenderBackend
{
	enum class ERenderTargetLoadOperation : uint8_t
	{
		Load = 0,
		Clear,
		DontCare
	};

	enum class ERenderTargetStoreOperation : uint8_t
	{
		Store = 0,
		DontCare
	};

	struct DepthStencilClearValue
	{
		// Reverse Z
		float			m_ClearDepth = 0.0f;
		uint8_t			m_ClearStencil = 0u;
	};
	
	struct RenderPassRenderTargetBinding
	{
		ERenderTargetLoadOperation							m_LoadOp = ERenderTargetLoadOperation::DontCare;
		ERenderTargetStoreOperation							m_StoreOp = ERenderTargetStoreOperation::Store;
		std::variant<glm::vec4, DepthStencilClearValue>		m_ClearValue;
	};
	
	// TODO: move to core or somewhere
	struct CommonColors
	{
		static glm::vec4 m_White;
		static glm::vec4 m_Black;
		static glm::vec4 m_Red;
		static glm::vec4 m_Green;
		static glm::vec4 m_Blue;
		static glm::vec4 m_Transparent;

		CommonColors() = delete;
	};
	
	// TODO: real render pass (use dynamic rendering currently, ignore framebuffer and renderpass)
	class RenderPass final : public RenderDeviceChild
	{
	public:
		
		static RenderPass* Create(RenderDevice& renderDevice);

	private:

		RenderPass(RenderDevice& renderDevice);
	
	private:

		VkRenderPass				m_Handle = nullptr;
	};
}

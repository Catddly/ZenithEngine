#include "RenderPass.h"

#include "RenderDevice.h"

namespace ZE::RenderBackend
{
	glm::vec4 CommonColors::m_White       = { 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec4 CommonColors::m_Black       = { 0.0f, 0.0f, 0.0f, 1.0f };
	glm::vec4 CommonColors::m_Red         = { 1.0f, 0.0f, 0.0f, 1.0f };
	glm::vec4 CommonColors::m_Green       = { 0.0f, 1.0f, 0.0f, 1.0f };
	glm::vec4 CommonColors::m_Blue        = { 0.0f, 0.0f, 1.0f, 1.0f };
	glm::vec4 CommonColors::m_Transparent = { 0.0f, 0.0f, 0.0f, 0.0f };
	
	RenderPass* RenderPass::Create(RenderDevice& renderDevice)
	{
		auto* pRenderPass = new RenderPass(renderDevice);
		return pRenderPass;
	}
	
	RenderPass::RenderPass(RenderDevice& renderDevice)
	{
		SetRenderDevice(&renderDevice);
	}
}

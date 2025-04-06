#include "TriangleRenderer.h"

#include "Render/RenderGraph.h"
#include "RenderBackend/RenderResource.h"

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace ZE::Renderer
{
	struct Vertex
	{
		float			m_Position[3];
		float			m_Color[3];
	};

	const std::vector<Vertex> vertices{
		{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
		{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
	};

	const std::vector<uint32_t> indices{ 0, 1, 2 };
	
	bool TriangleRenderer::Prepare(RenderBackend::RenderDevice& renderDevice)
	{
		RenderBackend::BufferDesc bufferDesc("triangle vertex buffer");
		bufferDesc.m_Size = static_cast<uint32_t>(sizeof(Vertex)) * vertices.size();
		bufferDesc.m_Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferDesc.m_MemoryUsage = RenderBackend::BufferMemoryUsage::GpuOnly;

		m_VertexBuffer = std::shared_ptr<RenderBackend::Buffer>(RenderBackend::Buffer::Create(renderDevice, bufferDesc, vertices.data(), bufferDesc.m_Size));
		if (!m_VertexBuffer)
		{
			return false;
		}

		bufferDesc.m_DebugName = "triangle index buffer";
		bufferDesc.m_Size = static_cast<uint32_t>(indices.size()) * sizeof(uint32_t);
		bufferDesc.m_Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferDesc.m_MemoryUsage = RenderBackend::BufferMemoryUsage::GpuOnly;

		m_IndexBuffer = std::shared_ptr<RenderBackend::Buffer>(RenderBackend::Buffer::Create(renderDevice, bufferDesc, indices.data(), bufferDesc.m_Size));
		if (!m_IndexBuffer)
		{
			return false;
		}

		m_TriangleVS = std::make_shared<RenderBackend::VertexShader>(renderDevice, "../ZenithEngine/Shaders/TriangleVS.spirv");
		m_TrianglePS = std::make_shared<RenderBackend::PixelShader>(renderDevice, "../ZenithEngine/Shaders/TrianglePS.spirv");

		if (!m_TriangleVS || !m_TrianglePS)
		{
			return false;
		}
			
		RenderBackend::Shader::LayoutBuilder vsBuilder(m_TriangleVS.get());
		vsBuilder.BindResource(0, "view", RenderBackend::EShaderBindingResourceType::UniformBuffer);
		vsBuilder.Build();

		RenderBackend::VertexShader::InputLayoutBuilder inputBuilder(m_TriangleVS.get());
		inputBuilder.AddLayout(VK_FORMAT_R32G32B32_SFLOAT, 12, 0);
		inputBuilder.AddLayout(VK_FORMAT_R32G32B32_SFLOAT, 12, 12);
		inputBuilder.Build();
		
		{
			auto pCmdList = renderDevice.GetImmediateCommandList();
			constexpr RenderBackend::ERenderResourceState prevAccess[] = {RenderBackend::ERenderResourceState::Undefined};
			
			constexpr RenderBackend::ERenderResourceState nextVBAccess[] = {RenderBackend::ERenderResourceState::VertexBuffer};
			constexpr RenderBackend::ERenderResourceState nextIBAccess[] = {RenderBackend::ERenderResourceState::IndexBuffer};

			RenderBackend::BufferBarrier bufferBarriers[2];

			bufferBarriers[0].m_Buffer = m_VertexBuffer->GetNativeHandle();
			bufferBarriers[0].m_Size = m_VertexBuffer->GetDesc().m_Size;
			bufferBarriers[0].m_Offset = 0;
			bufferBarriers[0].m_SrcQueueFamilyIndex = pCmdList->GetQueueIndex();
			bufferBarriers[0].m_DstQueueFamilyIndex = pCmdList->GetQueueIndex();
			bufferBarriers[0].m_PrevAccesses = prevAccess;
			bufferBarriers[0].m_NextAccesses = nextVBAccess;

			bufferBarriers[1].m_Buffer = m_IndexBuffer->GetNativeHandle();
			bufferBarriers[1].m_Size = m_IndexBuffer->GetDesc().m_Size;
			bufferBarriers[1].m_Offset = 0;
			bufferBarriers[1].m_SrcQueueFamilyIndex = pCmdList->GetQueueIndex();
			bufferBarriers[1].m_DstQueueFamilyIndex = pCmdList->GetQueueIndex();
			bufferBarriers[1].m_PrevAccesses = prevAccess;
			bufferBarriers[1].m_NextAccesses = nextIBAccess;
			
			pCmdList->BeginRecord();
			pCmdList->CmdResourceBarrier(nullptr, bufferBarriers, {});
			pCmdList->EndRecord();

			renderDevice.SubmitCommandListAndWaitUntilFinish(pCmdList.get());
		}

		return true;
	}
	
	void TriangleRenderer::Release(RenderBackend::RenderDevice& renderDevice)
	{
		m_VertexBuffer.reset();
		m_IndexBuffer.reset();
	}

	void TriangleRenderer::Render(Render::RenderGraph& renderGraph, Render::GraphResourceHandle outputColorRT, Render::GraphResourceHandle outputDepthRT)
	{
		using namespace ZE::Render;
		using namespace ZE::RenderBackend;
		
		auto vbHandle = renderGraph.ImportResource(m_VertexBuffer, ERenderResourceState::VertexBuffer);
		auto idHandle = renderGraph.ImportResource(m_IndexBuffer, ERenderResourceState::IndexBuffer);
		
		struct Matrices
		{
			glm::mat4 m_ModelMat;
			glm::mat4 m_ViewMat;
			glm::mat4 m_ProjectionMat;
		};

		BufferDesc uniformBufferDesc;
		uniformBufferDesc.m_Size = sizeof(Matrices);
		uniformBufferDesc.m_Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		uniformBufferDesc.m_MemoryUsage = BufferMemoryUsage::CpuToGpu;
		auto matrixBufferHandle = renderGraph.CreateResource(uniformBufferDesc);

		auto& drawTriangleNode = renderGraph.AddNode("Draw Triangle");

		drawTriangleNode.Read(vbHandle, ERenderResourceState::VertexBuffer);
		drawTriangleNode.Read(idHandle, ERenderResourceState::IndexBuffer);
		drawTriangleNode.Read(matrixBufferHandle, ERenderResourceState::VertexShaderReadUniformBuffer);
		
		drawTriangleNode.Write(outputColorRT, ERenderResourceState::ColorAttachmentWrite);
		drawTriangleNode.Write(outputDepthRT, ERenderResourceState::DepthStencilAttachmentWrite);
		
		drawTriangleNode
			.AddColorRenderTarget(outputColorRT, ERenderTargetLoadOperation::DontCare, ERenderTargetStoreOperation::Store, CommonColors::m_Black)
			.BindDepthStencilRenderTarget(outputDepthRT, ERenderTargetLoadOperation::DontCare, ERenderTargetStoreOperation::Store)
			.BindVertexShader(m_TriangleVS)
			.BindPixelShader(m_TrianglePS)
			.Execute([matrixBufferHandle, vbHandle, idHandle, outputColorRT](GraphExecutionContext& context)
		{
			const auto& desc = context.GetDesc<GraphResourceType::Texture>(outputColorRT);
			context.SetViewportSize(desc.m_Size.x, desc.m_Size.y);
			
			Matrices matrices;
			matrices.m_ModelMat = glm::mat4(1.0f);
			matrices.m_ViewMat = glm::lookAtRH(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			matrices.m_ProjectionMat = glm::infinitePerspectiveRH_ZO(glm::radians(45.0f), 1.0f, 1.0f);

			context.UpdateUniformBuffer(matrixBufferHandle, matrices);
			context.BindResource("view", matrixBufferHandle);
			context.BindPipeline();

			context.BindVertexInput(vbHandle, idHandle);
			context.DrawIndexed(3u, 1u, 0, 0, 0);
		});
	}
}

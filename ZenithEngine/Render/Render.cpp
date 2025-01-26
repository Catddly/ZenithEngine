#include "Render.h"

#include "RenderGraph.h"
#include "Core/Reflection.h"
#include "Platform/Displayable.h"
#include "RenderBackend/RenderDevice.h"
#include "RenderBackend/RenderWindow.h"

#include "RenderBackend/Shader.h"

namespace ZE::Render
{
	struct MyA
	{
		int             m_A;
		double          m_B;
	};

	struct MyB
	{
		double              m_NumberArray[12];
	};  

    bool RenderModule::InitializeModule()
    {
		m_pRenderDevice = new RenderBackend::RenderDevice(*this, RenderBackend::RenderDevice::Settings{});

        m_pMainRenderWindow = std::make_shared<RenderBackend::RenderWindow>(*m_pRenderDevice, Platform::Window::Settings{});

        if (m_pRenderDevice && !m_pRenderDevice->Initialize())
        {
            m_pRenderDevice->Shutdown();
            return false;
        }

        m_pMainRenderWindow->Initialize();

		//Render::RenderGraph graph(*m_pRenderDevice);

		//     {
//         RenderGraph::BufferResource bufferA{ .m_Id = 0, .m_Value = 2.33 };
//         RenderGraph::BufferResource bufferB{ .m_Id = 1, .m_Value = 5.485 };
//         RenderGraph::BufferResource bufferC{ .m_Id = 2, .m_Value = 95.656 };

//         RenderGraph::TextureResource texA{ .m_SerialNumberArray = { 1, 2, 3 } };
//         RenderGraph::TextureResource texB{ .m_SerialNumberArray = { 3, 2, 1 } };
//         RenderGraph::TextureResource texC{ .m_SerialNumberArray = { 1, 2, 3, 4, 5 } };
//         RenderGraph::TextureResource texD{ .m_SerialNumberArray = { 1, 2, 3, 0 } };

		 //auto& myA = graph.AllocateNodeResource<MyA>();
		 //auto& myB = graph.AllocateNodeResource<MyB>();

	  //   RenderGraph::GraphNode* pGraphNodeA = graph.AddNode("A");

//         pGraphNodeA->Read(bufferA);
//         pGraphNodeA->Read(bufferC);

//         pGraphNodeA->Write(texA);

//         pGraphNodeA->Execute([&myA] ()
//         {
		 //	myA.m_A = 3;
		 //	myA.m_B = 5.46;
//             ZE_LOG_INFO("Log from node A!");
//         });

		 //RenderGraph::GraphNode* pGraphNodeD = graph.AddNode("D");

		 //pGraphNodeD->Read(bufferC);

		 //auto OutputTexD = pGraphNodeD->Write(texD);

//         pGraphNodeD->Execute([&myA]()
		 //{
		 //	myA.m_A = 7;
		 //	myA.m_B = 5.46;
		 //	ZE_LOG_INFO("Log from node D!");
		 //});

	  //   RenderGraph::GraphNode* pGraphNodeB = graph.AddNode("B");

//         pGraphNodeB->Read(bufferA);

//         auto OutputTexB = pGraphNodeB->Write(texB);

//         pGraphNodeB->Execute([&myA]()
		 //{
		 //	myA.m_A = 1;
		 //	myA.m_B = 1.46;
		 //	ZE_LOG_INFO("Log from node B!");
		 //});

		 //RenderGraph::GraphNode* pGraphNodeC = graph.AddNode("C");

//         pGraphNodeC->Read(bufferA);
//         pGraphNodeC->Read(bufferB);
//         pGraphNodeC->Read(OutputTexB);

//         auto OutputTexC = pGraphNodeC->Write(texC);

//         pGraphNodeC->Execute([&myB]()
		 //{
		 //	myB.m_NumberArray[0] = 5.66;
		 //	myB.m_NumberArray[11] = 2.66;
		 //	ZE_LOG_INFO("Log from node C!");
		 //});

		 //RenderGraph::GraphNode* pGraphNodeE = graph.AddNode("E");

//         pGraphNodeE->Read(OutputTexC);
//         pGraphNodeE->Read(OutputTexD);

//         pGraphNodeE->Execute([&myB]()
//         {
//             myB.m_NumberArray[1] = 5.66;
//             myB.m_NumberArray[10] = 2.66;
//             ZE_LOG_INFO("Log from node E!");
//         });
//     }

		//graph.Execute();

		{
			m_pTriangleVS = std::make_shared<RenderBackend::VertexShader>(*m_pRenderDevice, "../ZenithEngine/Shaders/TriangleVS.spirv");
			m_pTrianglePS = std::make_shared<RenderBackend::PixelShader>(*m_pRenderDevice, "../ZenithEngine/Shaders/TrianglePS.spirv");

			ZE_CHECK(m_pTriangleVS);
			ZE_CHECK(m_pTrianglePS);
		}

        return true;
    }

    void RenderModule::ShutdownModule()
    {
		{
			m_pTriangleVS.reset();
			m_pTrianglePS.reset();
		}

        if (m_pMainRenderWindow)
        {
            m_pMainRenderWindow->Shutdown();
            m_pMainRenderWindow.reset();
        }

        if (m_pRenderDevice)
		{
			m_pRenderDevice->Shutdown();
			delete m_pRenderDevice;
		}
    }

    void RenderModule::BuildFrameTasks(tf::Taskflow& taskFlow)
    {
		tf::Task drawTask = taskFlow.emplace([this]()
		{
			Draw();
		});
    }

	void RenderModule::Draw()
	{
		using namespace ZE::Render;
		using namespace ZE::RenderBackend;

		RenderGraph renderGraph(*m_pRenderDevice);

		{
			auto& drawTriangleNode = renderGraph.AddNode("Draw Triangle");
	
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

			RenderBackend::BufferDesc bufferDesc;
			bufferDesc.m_Size = sizeof(Vertex) * vertices.size();
			bufferDesc.m_Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			bufferDesc.m_MemoryUsage = BufferMemoryUsage::GpuOnly;

			auto vertexBuffer = renderGraph.CreateResource(bufferDesc, vertices.data(), bufferDesc.m_Size);

			std::vector<uint32_t> indices{ 0, 1, 2 };
			bufferDesc.m_Size = indices.size() * sizeof(uint32_t);
			bufferDesc.m_Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			bufferDesc.m_MemoryUsage = BufferMemoryUsage::GpuOnly;

			auto indexBuffer = renderGraph.CreateResource(bufferDesc, indices.data(), bufferDesc.m_Size);
		
			drawTriangleNode.Read(vertexBuffer, RenderResourceState::VertexBuffer);
			drawTriangleNode.Read(indexBuffer, RenderResourceState::IndexBuffer);
			
			Texture* pSwapchainRT = GetMainRenderWindow()->GetSwapchainRenderTarget();
			ZE_CHECK(pSwapchainRT);
			RenderBackend::TextureDesc depthDesc;
			depthDesc.m_Size = pSwapchainRT->GetDesc().m_Size;
			depthDesc.m_Format = VK_FORMAT_D32_SFLOAT_S8_UINT;
			depthDesc.m_bIsDepthStencil = true;

			auto depthBuffer = renderGraph.CreateResource(depthDesc);

			drawTriangleNode
				.BindColor(depthBuffer)
				.BindDepthStencil(depthBuffer)
				.BindVertexShader(m_pTriangleVS)
				.BindPixelShader(m_pTrianglePS)
				.Execute([]()
			{

			});
		}

		renderGraph.Execute();
	}

    //std::shared_ptr<RenderBackend::RenderWindow> RenderModule::CreateSecondaryRenderWindow()
    //{
    //    return m_pRenderDevice->CreateSecondaryRenderWindow();
    //}
}

REFL_AUTO(type(ZE::Render::MyA), field(m_A), field(m_B))
REFL_AUTO(type(ZE::Render::MyB), field(m_NumberArray))

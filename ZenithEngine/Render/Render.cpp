#include "Render.h"

#include "RenderGraph.h"
#include "Core/Reflection.h"
#include "Platform/Displayable.h"
#include "RenderBackend/RenderDevice.h"
#include "RenderBackend/RenderWindow.h"

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
        RenderGraph::RenderGraph graph;

        {
            RenderGraph::BufferResource bufferA{ .m_Id = 0, .m_Value = 2.33 };
            RenderGraph::BufferResource bufferB{ .m_Id = 1, .m_Value = 5.485 };
            RenderGraph::BufferResource bufferC{ .m_Id = 2, .m_Value = 95.656 };

            RenderGraph::TextureResource texA{ .m_SerialNumberArray = { 1, 2, 3 } };
            RenderGraph::TextureResource texB{ .m_SerialNumberArray = { 3, 2, 1 } };
            RenderGraph::TextureResource texC{ .m_SerialNumberArray = { 1, 2, 3, 4, 5 } };
            RenderGraph::TextureResource texD{ .m_SerialNumberArray = { 1, 2, 3, 0 } };

			auto& myA = graph.AllocateNodeResource<MyA>();
			auto& myB = graph.AllocateNodeResource<MyB>();

		    RenderGraph::GraphNode* pGraphNodeA = graph.AddNode("A");

            pGraphNodeA->Read(bufferA);
            pGraphNodeA->Read(bufferC);

            pGraphNodeA->Write(texA);

            pGraphNodeA->Execute([&myA] ()
            {
				myA.m_A = 3;
				myA.m_B = 5.46;
                ZE_LOG_INFO("Log from node A!");
            });

			RenderGraph::GraphNode* pGraphNodeD = graph.AddNode("D");

			pGraphNodeD->Read(bufferC);

			auto OutputTexD = pGraphNodeD->Write(texD);

            pGraphNodeD->Execute([&myA]()
			{
				myA.m_A = 7;
				myA.m_B = 5.46;
				ZE_LOG_INFO("Log from node D!");
			});

		    RenderGraph::GraphNode* pGraphNodeB = graph.AddNode("B");

            pGraphNodeB->Read(bufferA);

            auto OutputTexB = pGraphNodeB->Write(texB);

            pGraphNodeB->Execute([&myA]()
			{
				myA.m_A = 1;
				myA.m_B = 1.46;
				ZE_LOG_INFO("Log from node B!");
			});

			RenderGraph::GraphNode* pGraphNodeC = graph.AddNode("C");

            pGraphNodeC->Read(bufferA);
            pGraphNodeC->Read(bufferB);
            pGraphNodeC->Read(OutputTexB);

            auto OutputTexC = pGraphNodeC->Write(texC);

            pGraphNodeC->Execute([&myB]()
			{
				myB.m_NumberArray[0] = 5.66;
				myB.m_NumberArray[11] = 2.66;
				ZE_LOG_INFO("Log from node C!");
			});

			RenderGraph::GraphNode* pGraphNodeE = graph.AddNode("E");

            pGraphNodeE->Read(OutputTexC);
            pGraphNodeE->Read(OutputTexD);

            pGraphNodeE->Execute([&myB]()
            {
                myB.m_NumberArray[1] = 5.66;
                myB.m_NumberArray[10] = 2.66;
                ZE_LOG_INFO("Log from node E!");
            });
        }

        graph.Execute();

        //-------------------------------------------------------------------------

        auto pRenderDevice = new RenderBackend::RenderDevice(*this, RenderBackend::RenderDevice::Settings{});
        m_pRenderDevice = pRenderDevice;

        m_pMainRenderWindow = std::make_shared<RenderBackend::RenderWindow>(*pRenderDevice, Platform::Window::Settings{});

        if (m_pRenderDevice && !m_pRenderDevice->Initialize())
        {
            m_pRenderDevice->Shutdown();
            return false;
        }

        m_pMainRenderWindow->Initialize();

        return true;
    }

    void RenderModule::ShutdownModule()
    {
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
    }

    //std::shared_ptr<RenderBackend::RenderWindow> RenderModule::CreateSecondaryRenderWindow()
    //{
    //    return m_pRenderDevice->CreateSecondaryRenderWindow();
    //}
}

REFL_AUTO(type(ZE::Render::MyA), field(m_A), field(m_B))
REFL_AUTO(type(ZE::Render::MyB), field(m_NumberArray))

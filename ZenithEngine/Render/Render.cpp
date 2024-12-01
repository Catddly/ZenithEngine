#include "Render.h"

#include "RenderGraph.h"

namespace ZE::Render
{
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

		    RenderGraph::GraphNode* pGraphNodeA = graph.AddNode("A");

            pGraphNodeA->Read(bufferA);
            pGraphNodeA->Read(bufferC);

            pGraphNodeA->Write(texA);

			RenderGraph::GraphNode* pGraphNodeD = graph.AddNode("D");

			pGraphNodeD->Read(bufferC);

			auto OutputTexD = pGraphNodeD->Write(texD);

		    RenderGraph::GraphNode* pGraphNodeB = graph.AddNode("B");

            pGraphNodeB->Read(bufferA);

            auto OutputTexB = pGraphNodeB->Write(texB);

			RenderGraph::GraphNode* pGraphNodeC = graph.AddNode("C");

            pGraphNodeC->Read(bufferA);
            pGraphNodeC->Read(bufferB);
            pGraphNodeC->Read(OutputTexB);

            auto OutputTexC = pGraphNodeC->Write(texC);

			RenderGraph::GraphNode* pGraphNodeE = graph.AddNode("E");

            pGraphNodeE->Read(OutputTexC);
            pGraphNodeE->Read(OutputTexD);
        }

        graph.Execute();

        return true;
    }

    void RenderModule::ShutdownModule()
    {
    }

    void RenderModule::BuildFrameTasks(tf::Taskflow& taskFlow)
    {

    }
}

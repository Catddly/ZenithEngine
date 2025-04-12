#include "Render.h"

#include "RenderGraph.h"
#include "RenderBackend/RenderDevice.h"
#include "RenderBackend/RenderWindow.h"
#include "RenderBackend/PipelineStateCache.h"
#include "RenderBackend/Shader.h"

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace ZE::Render
{
    bool RenderModule::InitializeModule()
    {
		m_RenderDevice = new RenderBackend::RenderDevice(*this, RenderBackend::RenderDevice::Settings{});

    	m_MainRenderWindow = std::make_shared<RenderBackend::RenderWindow>(*m_RenderDevice, Platform::Window::Settings{});
    	
        if (m_RenderDevice && !m_RenderDevice->Initialize())
        {
            m_RenderDevice->Shutdown();
        	delete m_RenderDevice;
            return false;
        }
    	
        m_MainRenderWindow->Initialize();

    	m_PipelineStateCache = new RenderBackend::PipelineStateCache(*m_RenderDevice);

    	m_TriangleRenderer.Prepare(*m_RenderDevice);

        return true;
    }

    void RenderModule::ShutdownModule()
    {
    	m_RenderDevice->WaitUntilIdle();

    	m_TriangleRenderer.Release(*m_RenderDevice);
    	
		delete m_PipelineStateCache;
    	
        if (m_MainRenderWindow)
        {
            m_MainRenderWindow->Shutdown();
            m_MainRenderWindow.reset();
        }

        if (m_RenderDevice)
		{
			m_RenderDevice->Shutdown();
			delete m_RenderDevice;
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
    	
		m_MainRenderWindow->BeginFrame();
    	// must wait for the commands to finish before releasing all defer release resources
		m_RenderDevice->BeginFrame();
    	
		RenderGraph renderGraph(*m_RenderDevice);
    	
    	auto pSwapchainRT = GetMainRenderWindow()->GetFrameSwapchainRenderTarget();
    	ZE_CHECK(pSwapchainRT);
    	TextureDesc depthDesc("depth render target");
    	depthDesc.m_Size = pSwapchainRT->GetDesc().m_Size;
    	depthDesc.m_Format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    	depthDesc.m_Usage = 1 << static_cast<uint8_t>(ETextureUsage::DepthStencil);

    	auto swapchainRTHandle = renderGraph.ImportResource(pSwapchainRT, ERenderResourceState::Present);
    	auto depthRTHandle = renderGraph.CreateResource(depthDesc);

    	m_TriangleRenderer.Render(renderGraph, swapchainRTHandle, depthRTHandle);

		{
			auto& presentNode = renderGraph.AddNode("Present");
			presentNode.Read(swapchainRTHandle, ERenderResourceState::Present);
		}
    	
		renderGraph.Execute(*m_PipelineStateCache, *m_MainRenderWindow);
    	m_MainRenderWindow->Present();

    	m_MainRenderWindow->EndFrame();
    	m_RenderDevice->EndFrame();
	}
}

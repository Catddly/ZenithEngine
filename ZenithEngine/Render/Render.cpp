#include "Render.h"

#include "RenderGraph.h"
#include "Render/Shader.h"
#include "RenderBackend/RenderDevice.h"
#include "RenderBackend/RenderWindow.h"
#include "RenderBackend/PipelineStateCache.h"
#include "Render/Loader/StaticMeshLoader.h"
#include "Render/Loader/ShaderLoader.h"

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "Core/Engine.h"
#include "Input/Input.h"

namespace ZE::Render
{
    bool RenderModule::InitializeModule()
    {
		m_RenderDevice = new RenderBackend::RenderDevice(*this, RenderBackend::RenderDevice::Settings{});

    	Asset::AssetManager::Get().RegisterAssetLoader<StaticMesh>(new StaticMeshLoader);
    	Asset::AssetManager::Get().RegisterAssetLoader<VertexShader>(new ShaderLoader(*m_RenderDevice));
    	Asset::AssetManager::Get().RegisterAssetLoader<PixelShader>(new ShaderLoader(*m_RenderDevice));
    	
    	m_MainRenderWindow = std::make_shared<RenderBackend::RenderWindow>(*m_RenderDevice, Platform::Window::Settings{});
    	
        if (m_RenderDevice && !m_RenderDevice->Initialize())
        {
            m_RenderDevice->Shutdown();
        	delete m_RenderDevice;
            return false;
        }
    	
        m_MainRenderWindow->Initialize();
    	GetEngine().GetInputModule()->AddWindow(m_MainRenderWindow.get());

    	m_PipelineStateCache = new RenderBackend::PipelineStateCache(*m_RenderDevice);

    	if (!m_TriangleRenderer.Prepare(*m_RenderDevice))
    	{
    		return false;
    	}

        return true;
    }

    void RenderModule::ShutdownModule()
    {
    	m_RenderDevice->WaitUntilIdle();

    	m_TriangleRenderer.Release(*m_RenderDevice);
    	
		delete m_PipelineStateCache;
    	
        if (m_MainRenderWindow)
        {
    		GetEngine().GetInputModule()->RemoveWindow(m_MainRenderWindow.get());
        	
            m_MainRenderWindow->Shutdown();
            m_MainRenderWindow.reset();
        }

        if (m_RenderDevice)
		{
			m_RenderDevice->Shutdown();
			delete m_RenderDevice;
		}
    }
	
    void RenderModule::Render()
    {
    	using namespace ZE::Render;
    	using namespace ZE::RenderBackend;
    	
    	m_MainRenderWindow->BeginFrame();
    	// must wait for the commands to finish before releasing all defer release resources
    	m_RenderDevice->BeginFrame();
    	
    	RenderGraph renderGraph(*m_RenderDevice);
    	
    	auto pSwapchainRT = GetMainRenderWindow()->GetFrameSwapchainRenderTarget();
    	ZE_ASSERT(pSwapchainRT);
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

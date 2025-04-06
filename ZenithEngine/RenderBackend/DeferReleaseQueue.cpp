#include "DeferReleaseQueue.h"

namespace ZE::RenderBackend
{
	void DeferReleaseQueue::DeferRelease(IDeferReleaseResource* pDeferReleaseResource)
	{
		if (pDeferReleaseResource)
		{
			m_ToBeReleaseResources.push_back(pDeferReleaseResource);
		}
	}
	
	void DeferReleaseQueue::DeferRelease(const DeferReleaseLifetimeResource<Buffer>& deferReleaseResource)
	{
		m_ToBeReleaseBuffers.push_back(deferReleaseResource);
	}
	
	void DeferReleaseQueue::DeferRelease(const DeferReleaseLifetimeResource<Texture>& deferReleaseResource)
	{
		m_ToBeReleaseTextures.push_back(deferReleaseResource);
	}

	void DeferReleaseQueue::ReleaseAllImmediately()
	{
		for (auto& pResource : m_ToBeReleaseResources)
		{
			pResource->Release();
		}
		m_ToBeReleaseResources.clear();

		for (auto& resource : m_ToBeReleaseBuffers)
		{
			resource.Release();
		}
		m_ToBeReleaseBuffers.clear();

		for (auto& resource : m_ToBeReleaseTextures)
		{
			resource.Release();
		}
		m_ToBeReleaseTextures.clear();
	}
	
}

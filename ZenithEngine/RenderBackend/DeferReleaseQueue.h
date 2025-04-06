#pragma once

#include "Core/Assertion.h"

#include <vector>
#include <memory>

#include "Render/Render.h"

namespace ZE::RenderBackend
{
	class Buffer;
	class Texture;
	
	class IDeferReleaseResource
	{
	public:

		virtual ~IDeferReleaseResource() = default;
		
		virtual void Release() = 0;
	};

	/* Resource which its lifetime is managed. */
	template <typename T>
	class DeferReleaseLifetimeResource : public IDeferReleaseResource, public std::shared_ptr<T>
	{
	public:

		DeferReleaseLifetimeResource(T* ptr)
			: std::shared_ptr<T>(ptr)
		{}
		DeferReleaseLifetimeResource(std::shared_ptr<T> ptr)
			: std::shared_ptr<T>(std::move(ptr))
		{}
		
		virtual void Release() override;
	};
	
	template <typename T>
	void DeferReleaseLifetimeResource<T>::Release()
	{
		ZE_CHECK(std::shared_ptr<T>::use_count() == 1);
		std::shared_ptr<T>::reset();
	}

	class DeferReleaseQueue
	{
	public:

		void DeferRelease(IDeferReleaseResource* pDeferReleaseResource);

		void DeferRelease(const DeferReleaseLifetimeResource<Buffer>& deferReleaseResource);
		void DeferRelease(const DeferReleaseLifetimeResource<Texture>& deferReleaseResource);

		void ReleaseAllImmediately();
			
	private:

		std::vector<IDeferReleaseResource*>						m_ToBeReleaseResources;
		std::vector<DeferReleaseLifetimeResource<Buffer>>		m_ToBeReleaseBuffers;
		std::vector<DeferReleaseLifetimeResource<Texture>>		m_ToBeReleaseTextures;
	};
}

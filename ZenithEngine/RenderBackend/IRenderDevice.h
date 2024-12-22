#pragma once

#include <memory>

namespace ZE::RenderBackend
{
	class RenderWindow;

	class IRenderDevice
	{
	public:

		virtual ~IRenderDevice() = default;

		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;

		//virtual std::shared_ptr<RenderBackend::RenderWindow> CreateSecondaryRenderWindow() = 0;

	private:

	};
}
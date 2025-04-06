#pragma once

namespace ZE::RenderBackend
{
	class RenderWindow;

	class IRenderDevice
	{
	public:

		virtual ~IRenderDevice() = default;

		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;
	
	private:

	};
}
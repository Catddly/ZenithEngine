#pragma once

namespace ZE::RenderBackend
{
	class RenderDevice;

	class RenderDeviceChild
	{
	public:

		void SetRenderDevice(RenderDevice* pRenderDevice) { m_RenderDevice = pRenderDevice; }
		
		RenderDevice& GetRenderDevice() const { return *m_RenderDevice; }
		
	private:

		RenderDevice*				m_RenderDevice = nullptr;
	};	
}

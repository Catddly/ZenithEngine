#pragma once

#include "IDisplayable.h"

namespace ZE::Platform
{
	class Displayable : public IDisplayable
	{
	public:
		
		virtual bool Initialize() override;
		virtual void Shutdown() override;

		virtual void ProcessPlatformEvents() override;

	private:

		bool							m_IsInitialized = false;
	};
}
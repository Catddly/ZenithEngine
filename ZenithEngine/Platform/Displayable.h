#pragma once

#include "IDisplayable.h"

namespace ZE::Platform
{
	class Displayble : public IDisplayable
	{
	public:

		virtual ~Displayble() = default;

		virtual bool Initialize() override;
		virtual void Shutdown() override;

		virtual void ProcessPlatformEvents() override;

	private:

		bool							m_IsInitialized = false;
	};
}
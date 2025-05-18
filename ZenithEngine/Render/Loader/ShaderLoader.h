#pragma once

#include "Asset/AssetLoader.h"

#include <type_traits>

namespace ZE::RenderBackend { class RenderDevice; }
namespace ZE::Asset { class AssetRequest; }

namespace ZE::Render
{
	class ShaderLoader : public Asset::IAssetLoader
	{
	public:

		ShaderLoader(RenderBackend::RenderDevice& renderDevice);
		
		virtual bool Load(const Core::FilePath& filePath, Asset::AssetRequest& pAssetRequest) override;

	private:

		std::reference_wrapper<RenderBackend::RenderDevice>			m_RenderDevice;
	};
}

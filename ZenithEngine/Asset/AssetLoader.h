#pragma once

#include "Core/FileSystem.h"

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>

namespace ZE::Asset
{
	class AssetRequest;
	class Asset;

	class IAssetLoader
	{
	public:

		virtual ~IAssetLoader() = default;

		virtual bool Load(const Core::FilePath& filePath, AssetRequest& pAssetRequest) = 0;
	};
	
	class AssimpIOSystem final : public Assimp::IOSystem
	{
	public:

		virtual ~AssimpIOSystem() override = default;
		
		bool Exists(const char* pFile) const override;
		char getOsSeparator() const override;
		Assimp::IOStream* Open(const char* pFile, const char* pMode) override;
		void Close(Assimp::IOStream* pFile) override;
	};
}

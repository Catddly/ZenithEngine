#pragma once

#include "Asset/AssetLoader.h"

#include <assimp/Importer.hpp>
#include <glm/fwd.hpp>

struct aiScene;
struct aiNode;
struct aiMesh;

namespace ZE::Render
{
	class AssetRequest;
	class StaticMesh;
	
	class StaticMeshLoader : public Asset::IAssetLoader
	{
	public:
		
		virtual bool Load(const Core::FilePath& filePath, Asset::AssetRequest& pAssetRequest) override;

	private:

		void ReadNode(const aiScene* pScene, aiNode* pNode, StaticMesh* pAsset, glm::mat4 accumTransform);
		void ReadMesh(aiMesh* pMesh, StaticMesh* pAsset);
	};
}
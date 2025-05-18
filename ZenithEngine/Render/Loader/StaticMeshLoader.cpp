#include "StaticMeshLoader.h"

#include "Log/Log.h"
#include "Core/Assertion.h"
#include "Render/StaticMesh.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/ext/matrix_transform.hpp>

#include "Asset/AssetManager.h"

namespace ZE::Render
{
	namespace 
	{
		glm::mat4 ToEngineMat4(const aiMatrix4x4& mat)
		{
			glm::mat4 result;
			result[0][0] = mat.a1;
			result[0][1] = mat.a2;
			result[0][2] = mat.a3;
			result[0][3] = mat.a4;
			
			result[1][0] = mat.b1;
			result[1][1] = mat.b1;
			result[1][2] = mat.b1;
			result[1][3] = mat.b1;
			
			result[2][0] = mat.c1;
			result[2][1] = mat.c1;
			result[2][2] = mat.c1;
			result[2][3] = mat.c1;
			
			result[3][0] = mat.d1;
			result[3][1] = mat.d1;
			result[3][2] = mat.d1;
			result[3][3] = mat.d1;
			return result;
		}
	}

	bool StaticMeshLoader::Load(const Core::FilePath& filePath, Asset::AssetRequest& pAssetRequest)
	{
		Assimp::Importer importer;
		importer.SetIOHandler(new Asset::AssimpIOSystem);
		
		const aiScene* pScene = importer.ReadFile(
			filePath.ToString().c_str(),
			aiProcess_JoinIdenticalVertices | aiProcess_GenBoundingBoxes);
			
		if (!pScene || pScene->mNumMeshes == 0)
		{
			ZE_LOG_ERROR("Failed to load asset! [{}]", importer.GetErrorString());
			return false;
		}

		auto* pStaticMesh = new StaticMesh;
		ReadMesh(pScene->mMeshes[0], pStaticMesh);

		// auto transform = glm::identity<glm::mat4>();
		//
		// transform = ToEngineMat4(pScene->mRootNode->mTransformation);
		// ReadNode(pScene, pScene->mRootNode, pStaticMesh, transform);

		pAssetRequest.SetAsset(pStaticMesh);
		return true;
	}

	void StaticMeshLoader::ReadNode(const aiScene* pScene, aiNode* pNode, StaticMesh* pAsset, glm::mat4 accumTransform)
	{
		for (auto i = 0u; i < pNode->mNumMeshes; ++i)
		{
			ZE_ASSERT(i > 0);
			auto meshIndex = pNode->mMeshes[i];
			auto* pMesh = pScene->mMeshes[meshIndex];
			ReadMesh(pMesh, pAsset);
		}

		ZE_UNIMPLEMENTED();
		
		// for (auto i = 0u; i < pNode->mNumChildren; ++i)
		// {
		// 	auto* pNode = pNode->mChildren[i];
		// 	// pNode->
		// }
	}
	
	void StaticMeshLoader::ReadMesh(aiMesh* pMesh, StaticMesh* pAsset)
	{
		ZE_ASSERT(!pMesh->HasBones() && pMesh->HasPositions());
		ZE_ASSERT(pMesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);
		
		pAsset->m_AABB.SetMin({ pMesh->mAABB.mMin.x, pMesh->mAABB.mMin.y, pMesh->mAABB.mMin.z });
		pAsset->m_AABB.SetMax({ pMesh->mAABB.mMax.x, pMesh->mAABB.mMax.y, pMesh->mAABB.mMax.z });
		
		pAsset->m_Vertices.resize(pMesh->mNumVertices);
		pAsset->m_Indices.reserve(pMesh->mNumFaces * 3ull);

		uint32_t i = 0;
		std::ranges::for_each(pAsset->m_Vertices, [&i, pMesh](auto& vertex)
		{
			vertex.m_Position = { pMesh->mVertices[i].x, pMesh->mVertices[i].y, pMesh->mVertices[i].z };
			if (pMesh->mNumUVComponents[0] > 0u)
			{
				vertex.m_UV = { pMesh->mTextureCoords[0][i].x, pMesh->mTextureCoords[0][i].y };
			}
			++i;
		});

		for (auto i = 0u; i < pMesh->mNumFaces; ++i)
		{
			auto& face = pMesh->mFaces[i];
			ZE_ASSERT(face.mNumIndices == 3);

			for (auto j = 0u; j < face.mNumIndices; ++j)
			{
				pAsset->m_Indices.push_back(face.mIndices[j]);
			}
		}
	}
}

#include "AssetManager.h"

#include "Render/Shader.h"
#include "Render/StaticMesh.h"
#include "Render/Loader/ShaderLoader.h"
#include "Render/Loader/StaticMeshLoader.h"

namespace ZE::Asset
{
	void AssetRequest::AsyncLoad()
	{
		SetLoadPhase(EAssetLoadPhase::Loading);
				
		auto& taskSystem = TaskSystem::TaskManager::Get();
		m_AsyncLoadTaskHandle = taskSystem.RunTask([this]
		{
			if (!m_Loader->Load(m_Id.GetPath(), *this))
			{
				SetLoadPhase(EAssetLoadPhase::Failed);
				ZE_LOG_ERROR("Failed to load asset: {}", m_Id.GetPath().ToString().c_str());
			}
			else
			{
				SetLoadPhase(EAssetLoadPhase::Loaded);
			}
		});
	}
	
	void AssetRequest::WaitUntilLoaded() const
	{
		m_AsyncLoadTaskHandle.Wait();
	}

	AssetManager::~AssetManager()
	{
		for (AssetRequest* pRequest : std::views::values(m_RequestedAssetMap))
		{
			delete pRequest->m_Asset;
			delete pRequest;
		}
		m_AssetLoaderMap.clear();
		
		for (auto& pLoader : std::views::values(m_AssetLoaderMap))
		{
			delete pLoader;
		}
		m_AssetLoaderMap.clear();
	}
	
	AssetManager& AssetManager::Get()
	{
		static AssetManager sAssetManager;
		return sAssetManager;
	}
	
	void AssetManager::Update()
	{
		// TODO: [optimize] minimize critical section
		std::lock_guard guard(m_Mutex);
		
		UpdateLoadingRequests();
		
		for (auto& pendingRequest : m_PendingRequests)
		{
			pendingRequest->AsyncLoad();
		}
		m_LoadingRequests[m_CurrentLoadingRequestIndex] = std::move(m_PendingRequests);
	}
	
	void AssetManager::WaitUntilAllRequestsFinished()
	{
		// TODO: [optimize] minimize critical section
		std::lock_guard guard(m_Mutex);

		// call update to immediately dispatch new requests
		Update();
		
		for (const auto* pLoadingRequest : std::move(m_LoadingRequests[m_CurrentLoadingRequestIndex]))
		{
			pLoadingRequest->WaitUntilLoaded();
		}
	}

	void AssetManager::UpdateLoadingRequests()
	{
		const uint8_t localIndex = m_CurrentLoadingRequestIndex;
		const uint8_t localNextIndex = (m_CurrentLoadingRequestIndex + 1) % 2;
		
		for (auto* pLoadingRequest : m_LoadingRequests[localIndex])
		{
			if (const auto result = pLoadingRequest->GetLoadPhase(); result == EAssetLoadPhase::Loading)
			{
				m_LoadingRequests[localNextIndex].push_back(pLoadingRequest);
			}
			else if (result == EAssetLoadPhase::Failed)
			{
				// TODO: failure fallback
				ZE_ASSERT(false);
			}
			else if (result == EAssetLoadPhase::Unloaded)
			{
				ZE_LOG_FATAL("Unexcepted load phase when updating loading requests.");
				ZE_UNREACHABLE();
			}
		}

		m_CurrentLoadingRequestIndex = localNextIndex;
	}
	
	void AssetManager::RequestLoad(AssetPtrBase& assetPtr)
	{
		if (!assetPtr.HasSetPath())
		{
			ZE_LOG_WARNING("Request to load empty path asset.");
			return;
		}
		
		std::lock_guard guard(m_Mutex);
		if (!m_RequestedAssetMap.contains(assetPtr.m_Id))
		{
			auto* pLoader = FindAssetLoader(assetPtr.GetAssetTypeName());
			if (!pLoader)
			{
				ZE_LOG_WARNING("Unknown resource type: {}. Can't find a valid loader for it.", assetPtr.GetAssetTypeName());
			}
			
			AssetRequest* pRequest = new AssetRequest;
			pRequest->m_Id = assetPtr.m_Id;
			pRequest->m_Loader = pLoader;

			assetPtr.m_AssetRequest = pRequest;
			m_RequestedAssetMap.emplace(assetPtr.m_Id, pRequest);
			m_PendingRequests.push_back(pRequest);
		}
	}
	
	IAssetLoader* AssetManager::FindAssetLoader(const std::string& assetTypeName)
	{
		if (auto iter = m_AssetLoaderMap.find(assetTypeName); iter != m_AssetLoaderMap.end())
		{
			return iter->second;
		}
		return nullptr;
	}
	
	void AssetManager::RegisterAssetLoader(std::string_view assetTypeName, IAssetLoader* pLoader)
	{
		std::lock_guard guard(m_Mutex);
		if (auto iter = m_AssetLoaderMap.find(std::string(assetTypeName)); iter != m_AssetLoaderMap.end())
		{
			const auto* pOldLoader = iter->second;
			delete pOldLoader;
			iter->second = pLoader;
		}
		else
		{
			m_AssetLoaderMap.emplace(assetTypeName, pLoader);
		}
	}
}

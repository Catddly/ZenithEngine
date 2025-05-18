#pragma once

#include "Core/Event.h"
#include "AssetUrl.h"
#include "TaskSystem/TaskManager.h"

#include <shared_mutex>
#include <unordered_map>

namespace ZE::Asset
{
	class Asset;
	class IAssetLoader;
	
	enum class EAssetLoadPhase : uint8_t
	{
		Unloaded,
		Loading,
		Loaded,
		Failed
	};
	
	class AssetRequest
	{
		friend class AssetManager;
		
	public:

		void AsyncLoad();
		void WaitUntilLoaded() const;

		void SetAsset(Asset* pAsset) { m_Asset = pAsset; }
		Asset* GetAsset() const { return m_Asset; }

		void SetLoadPhase(EAssetLoadPhase loadPhase) { m_LoadPhase.store(loadPhase, std::memory_order::release); }
		EAssetLoadPhase GetLoadPhase() const { return m_LoadPhase.load(std::memory_order::acquire); }
		
		bool IsUnloaded() const { return m_LoadPhase.load(std::memory_order::acquire) == EAssetLoadPhase::Unloaded; }
		bool IsLoading() const { return m_LoadPhase.load(std::memory_order::acquire) == EAssetLoadPhase::Loading; }
		bool IsLoaded() const { return m_LoadPhase.load(std::memory_order::acquire) == EAssetLoadPhase::Loaded; }
	
	private:

		std::atomic<EAssetLoadPhase>				m_LoadPhase = EAssetLoadPhase::Unloaded;
		AssetId										m_Id;
		IAssetLoader*								m_Loader = nullptr;
		Asset*										m_Asset = nullptr;
		TaskSystem::TaskHandle						m_AsyncLoadTaskHandle;
	};

	class AssetPtrBase;
	
	template <typename T>
	class AssetPtr;
	
	class AssetManager final
	{
	public:

		~AssetManager();
		
		ZE_NON_COPYABLE_AND_NON_MOVABLE_CLASS(AssetManager);

		static AssetManager& Get();

		template <typename T>
		void RegisterAssetLoader(IAssetLoader* pLoader) { RegisterAssetLoader(Core::GetTypeName_Direct<T>(), pLoader); }

		// Only main thread can call update
		void Update();
		void WaitUntilAllRequestsFinished();
		
		void RequestLoad(AssetPtrBase& assetPtr);

	private:

		void UpdateLoadingRequests();
		
		IAssetLoader* FindAssetLoader(const std::string& assetTypeName);

	private:

		AssetManager() = default;
		
		void RegisterAssetLoader(std::string_view assetTypeName, IAssetLoader* pLoader);
	
	private:

		std::unordered_map<AssetId, AssetRequest*>				m_RequestedAssetMap;
		std::unordered_map<std::string, IAssetLoader*>			m_AssetLoaderMap;

		std::vector<AssetRequest*>								m_PendingRequests;
		std::vector<AssetRequest*>								m_LoadingRequests[2];
		uint8_t													m_CurrentLoadingRequestIndex = 0;			

		std::recursive_mutex									m_Mutex;
	};

}

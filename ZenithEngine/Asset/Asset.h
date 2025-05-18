#pragma once

#include "Core/FileSystem.h"
#include "Core/Hash.h"
#include "Math/AxisAlignedBoundingBox.h"
#include "TaskSystem/TaskManager.h"
#include "AssetUrl.h"
#include "Asset/AssetManager.h"

#include <glm/vec2.hpp>

#include <memory>
#include <vector>
#include <type_traits>

namespace ZE::Asset
{
	// Asset relative features:
	// 1. Asset editor
	// 2. Object System -- Component-based system
	// 3. Serialization/Deserialization
	// 4. Async Loading (Any thread can check whether loading is complete and user can perform next step, all steps should be thread-safe)

	class Asset : public Core::IReflectable
	{
		friend class AssetManager;

		ZE_CLASS_REFL(Asset)
		
	public:

		Asset() = default;
		virtual ~Asset() = default;

		ZE_NON_COPYABLE_CLASS(Asset);
	};
	
	class AssetPtrBase
	{
		friend class AssetManager;
		
	public:

		AssetPtrBase() = default;
		AssetPtrBase(AssetId assetId)
			: m_Id{ std::move(assetId) }
		{}
		AssetPtrBase(const AssetPtrBase& other) { operator=(other); }
		AssetPtrBase(AssetPtrBase&& other) noexcept { operator=(std::move(other)); }
		
		void SetPath(const Core::FilePath& filePath) { m_Id.SetPath(filePath); }

		bool HasSetPath() const { return m_Id.IsValid(); }
		void WaitUntilLoaded() const { m_AssetRequest->WaitUntilLoaded(); }

		std::string GetAssetTypeName() const { return m_Id.GetAssetTypeName(); }

		EAssetLoadPhase GetLoadPhase() const { return m_AssetRequest ? m_AssetRequest->GetLoadPhase() : EAssetLoadPhase::Unloaded; }
		bool IsUnloaded() const { return GetLoadPhase() == EAssetLoadPhase::Unloaded; }
		bool IsLoading() const { return GetLoadPhase() == EAssetLoadPhase::Loading; }
		bool IsLoaded() const { return GetLoadPhase() == EAssetLoadPhase::Loaded; }

		AssetPtrBase& operator=(const AssetPtrBase& other)
		{
			ZE_ASSERT(!m_AssetRequest || m_AssetRequest->IsUnloaded());
			if (&other == this)
			{
				return *this;
			}
			
			m_Id = other.m_Id;
			m_AssetRequest = other.m_AssetRequest;
			return *this;
		}

		AssetPtrBase& operator=(AssetPtrBase&& other) noexcept
		{
			ZE_ASSERT(!m_AssetRequest || m_AssetRequest->IsUnloaded());
			if (&other == this)
			{
				return *this;
			}
			
			m_Id = other.m_Id;
			m_AssetRequest = other.m_AssetRequest;
			other.m_Id = {};
			other.m_AssetRequest = nullptr;
			return *this;
		}
		
	protected:

		AssetId								m_Id;
		const AssetRequest*					m_AssetRequest = nullptr;
	};

	template <typename T>
	concept AssetType = requires
	{
		requires std::is_base_of_v<Asset, T>;
	};
	
	template <typename T>
	class AssetPtr : public AssetPtrBase
	{
		static_assert(AssetType<T>);
		
	public:

		AssetPtr() = default;
		AssetPtr(const Core::FilePath& filePath)
			: AssetPtrBase{ AssetId{filePath, Core::GetTypeName_Direct<T>()} }
		{}
		AssetPtr(AssetId id)
			: AssetPtrBase{ std::move(id) }
		{}
		AssetPtr(const AssetPtrBase& other) { operator=(other); }
		AssetPtr(AssetPtrBase&& other) noexcept { operator=(other); other.m_Id = {}; other.m_AssetRequest = nullptr; }
		
		const T* operator->() const { ZE_ASSERT(m_AssetRequest); return reinterpret_cast<const T*>(m_AssetRequest->GetAsset()); }
		const T* GetAsset() const { ZE_ASSERT(m_AssetRequest); return reinterpret_cast<const T*>(m_AssetRequest->GetAsset()); }

		bool operator==(nullptr_t) const { return m_AssetRequest == nullptr; }
		bool operator!=(nullptr_t) const { return m_AssetRequest != nullptr; }
		bool operator==(AssetPtr const& rhs) const { return m_Id == rhs.m_Id; }
		bool operator!=(AssetPtr const& rhs) const { return m_Id != rhs.m_Id; }
		
		template <AssetType U>
		bool operator==(const AssetPtr<U>& rhs) { return m_Id == rhs.m_Id; }
		template <AssetType U>
		bool operator!=(const AssetPtr<U>& rhs) { return m_Id != rhs.m_Id; }

		AssetPtr& operator=(const AssetPtrBase& other)
		{
			// TODO: asset is now loaded once and never get released during runtime. Unload mechanism is needed
			ZE_ASSERT(!m_AssetRequest || m_AssetRequest->IsUnloaded());
			if (&other == this)
			{
				return *this;
			}

			if (Core::GetTypeName_Direct<T>() == other.GetAssetTypeName())
			{
				AssetPtrBase::operator=(*this, other);
			}
			else
			{
				ZE_LOG_FATAL("Try to assign a different type of asset ptr!");	
			}
			return *this;
		}
		
	private:
	};
}

REFL_AUTO(type(ZE::Asset::Asset))

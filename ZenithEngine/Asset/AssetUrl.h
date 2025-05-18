#pragma once

#include "Core/FileSystem.h"
#include "Core/Hash.h"

namespace ZE::Asset
{
	class AssetTypeId
	{
	public:

		AssetTypeId() = default;
		AssetTypeId(std::string_view resourceTypeName)
			: m_AssetTypeName{ resourceTypeName }
		{}
		
		uint64_t GetHash() const { return Core::Hash(m_AssetTypeName); }
		std::string GetAssetTypeName() const { return m_AssetTypeName; }

		auto operator<=>(const AssetTypeId&) const = default;
		bool operator==(const AssetTypeId&) const = default;

	private:

		std::string							m_AssetTypeName;
	};
	
	// Universal id for engine to recognize an asset
	class AssetId
	{
	public:

		AssetId() = default;
		AssetId(Core::FilePath path, std::string_view resourceTypeName)
			: m_Path{std::move(path)}, m_TypeId{ resourceTypeName }
		{}

		void SetPath(const Core::FilePath& path) { m_Path = path; }
		void SetAssetTypeId(const AssetTypeId& typeId) { m_TypeId = typeId; }

		bool IsValid() const { return m_Path.IsValid(); }
		uint64_t GetHash() const { return Core::Hash(m_Path) ^ m_TypeId.GetHash(); }
		Core::FilePath GetPath() const { return m_Path; }
		std::string GetAssetTypeName() const { return m_TypeId.GetAssetTypeName(); }

		auto operator<=>(const AssetId&) const = default;
		bool operator==(const AssetId&) const = default;
		
	private:

		Core::FilePath							m_Path{};
		AssetTypeId								m_TypeId;
	};

}

template <>
struct std::hash<ZE::Asset::AssetTypeId>
{
	std::size_t operator()(const ZE::Asset::AssetTypeId& assetTypeId) const noexcept
	{
		return assetTypeId.GetHash();
	}
};	

template <>
struct std::hash<ZE::Asset::AssetId>
{
	std::size_t operator()(const ZE::Asset::AssetId& assetId) const noexcept
	{
		return assetId.GetHash();
	}
};	

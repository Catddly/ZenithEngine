#include "AssetLoader.h"

#include "Asset.h"
#include "Core/Assertion.h"
#include "Log/Log.h"

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/scene.h>

namespace ZE::Asset
{
	class AssimpIOStream final : public Assimp::IOStream
	{
		friend class AssimpIOSystem;
		
	public:

		virtual ~AssimpIOStream() override = default;
		
		size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override
		{
			if (!m_Stream.read(static_cast<char*>(pvBuffer), static_cast<long long>(pSize * pCount)))
			{
				return 0;
			}
			return pCount;
		}
		
		size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) override
		{
			if (!m_Stream.write(static_cast<const char*>(pvBuffer), static_cast<long long>(pSize * pCount)))
			{
				return 0;
			}
			return pCount;
		}
		
		aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override
		{
			auto pos = 0;
			if (pOrigin == aiOrigin_SET)
			{
				pos = std::fstream::beg;
			}
			else if (pOrigin == aiOrigin_CUR)
			{
				pos = std::fstream::cur;
			}
			else if (pOrigin == aiOrigin_END)
			{
				pos = std::fstream::end;
			}
			
			if (!m_Stream.seekg(static_cast<long long>(pOffset), pos))
			{
				return aiReturn_FAILURE;
			}
			return aiReturn_SUCCESS;
		}

		size_t Tell() const override
		{
			return m_Stream.tellg();
		}
		
		size_t FileSize() const override
		{
			return m_FilePath.GetFileSize();
		}
		
		void Flush() override
		{
			m_Stream.flush();
		}

	private:

		Core::FilePath						m_FilePath;
		mutable std::fstream				m_Stream;
	};

	bool AssimpIOSystem::Exists(const char* pFile) const
	{
		auto path = Core::FilePath(pFile);
		Core::FileSystem::Sanitize(path);
		path = Core::FileSystem::ToAbsoluteEnginePath(path);
		return path.IsExist();
	}
		
	char AssimpIOSystem::getOsSeparator() const
	{
		return std::filesystem::path::preferred_separator;
	}
		
	Assimp::IOStream* AssimpIOSystem::Open(const char* pFile, const char* pMode)
	{
		AssimpIOStream* pStream = new AssimpIOStream;
			
		auto path = Core::FilePath(pFile);
		Core::FileSystem::Sanitize(path);
		path = Core::FileSystem::ToAbsoluteEnginePath(path);
			
		int modeFlag = 0;
		for (auto i = 0ull; i < strlen(pMode); ++i)
		{
			const char c = pMode[i];
			if (c == 'r')
			{
				modeFlag |= std::ios::in;
			}
			else if (c == 'w')
			{
				modeFlag |= std::ios::out;
			}
			else if (c == 'b')
			{
				modeFlag |= std::ios::binary;
			}
			else
			{
				ZE_UNIMPLEMENTED();
			}
		}

		pStream->m_Stream.open(path.ToString().c_str(), modeFlag);
		pStream->m_FilePath = path;
			
		return pStream;
	}
		
	void AssimpIOSystem::Close(Assimp::IOStream* pFile)
	{
		auto pLocalFile = static_cast<AssimpIOStream*>(pFile);
		pLocalFile->m_Stream.close();
		delete pFile;
	}
}

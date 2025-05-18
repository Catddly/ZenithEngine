#include "FileSystem.h"

#include "Log/Log.h"

#include <fstream>

namespace ZE::Core
{
    std::filesystem::path FileSystem::m_EngineMountPath{};

    FilePath::FilePath(const char* path)
        : m_Path(path)
    {}
    
    FilePath::FilePath(std::string_view path)
        : m_Path{path}
    {}
    
    FilePath::FilePath(std::filesystem::path path)
        : m_Path{std::move(path)}
    {}
    
    FileHandle FileHandle::FailedToOpen()
    {
        FileHandle handle;
        handle.m_HadOpen = false;
        return handle;
    }
    
    void FileSystem::Mount()
    {
        // TODO: is this robust?
        m_EngineMountPath = std::filesystem::absolute(std::filesystem::current_path().parent_path()).make_preferred();
    }
    
    FileHandle FileSystem::Load(const FilePath& filePath)
    {
        // TODO: support memory-mapped file (i.e. mmap)
        // This allows us to directly map memory to disk storage, without copy the disk binary into the ram and then read/write it
        // This reduce the memory copy operation and it is more efficient

        auto path = filePath.m_Path;
        Sanitize(path);
        
        std::error_code errorCode;
        const auto absolutePath = ToAbsoluteEnginePath(path);
        if (!std::filesystem::exists(absolutePath, errorCode))
        {
            ZE_LOG_ERROR("File [{}] didn't exists!", absolutePath.string());
            ZE_LOG_ERROR("error code: {}", errorCode.message());
            return FileHandle::FailedToOpen();
        }

        FileHandle handle;

        auto fileSize = std::filesystem::file_size(absolutePath);
        std::ifstream inFileStream(absolutePath, std::ios::in | std::ios::binary);
        if (inFileStream.is_open())
        {
            std::vector<std::byte> binary(fileSize);
            inFileStream.read(reinterpret_cast<char*>(binary.data()), static_cast<long long>(fileSize));

            handle.m_Binary = std::move(binary);
            handle.m_HadOpen = true;
        }
        else
        {
            ZE_LOG_ERROR("Failed to open [{}] for read!", absolutePath.string().c_str());
        }
        inFileStream.close();

        return handle;
    }
    
    FilePath FileSystem::ToAbsoluteEnginePath(const FilePath& filePath)
    {
        return ToAbsoluteEnginePath(filePath.m_Path);
    }
    
    bool FileSystem::IsSanitized(const FilePath& path)
    {
        if (!path.m_Path.is_relative())
        {
            return false;
        }

        if (!path.m_Path.has_root_directory())
        {
            return false;
        }

        return true;
    }

    void FileSystem::Sanitize(FilePath& path)
    {
        Sanitize(path.m_Path);
    }
    
    void FileSystem::Sanitize(std::filesystem::path& path)
    {
        if (!path.is_relative())
        {
            auto engineRelativePath = path.lexically_relative(m_EngineMountPath);
            if (engineRelativePath.empty())
            {
                ZE_LOG_WARNING("Failed to satinize path: {}. This path isn't inside engine mount path.");
            }
            path = std::string{std::filesystem::path::preferred_separator};
            path += engineRelativePath;
        }

        if (!path.has_root_directory())
        {
            auto newPath = std::filesystem::path{std::string{std::filesystem::path::preferred_separator}};
            newPath += path;
            path = newPath;
        }
    }

    std::filesystem::path FileSystem::ToAbsoluteEnginePath(std::filesystem::path filePath)
    {
        auto path = m_EngineMountPath;
        path += filePath;
        return path.make_preferred();
    }
}

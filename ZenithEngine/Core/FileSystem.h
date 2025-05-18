#pragma once

#include <filesystem>

namespace ZE::Core
{
    // User relative path.
    // Use as /ZenithEngine/Shaders/TriangleVS.spirv etc.
    class FilePath
    {
        friend class FileSystem;
        
    public:

        FilePath() = default;
        FilePath(const char* path);
        FilePath(std::string_view path);
        FilePath(std::filesystem::path path);
        
        uint64_t GetHash() const { return std::hash<std::filesystem::path>{}(m_Path); }
        bool IsValid() const { return !m_Path.empty(); }

        std::string ToString() const { return m_Path.string(); }

        bool IsExist() const { return std::filesystem::exists(m_Path); }
        bool IsAbsolute() const { return m_Path.is_absolute(); }
        bool IsRelative() const { return m_Path.is_relative(); }
        
        std::size_t GetFileSize() const { return std::filesystem::file_size(m_Path); }
        std::string GetExtension() const { return m_Path.extension().string(); }
        std::string GetFilename() const { return m_Path.filename().string(); }

        auto operator<=>(const FilePath&) const = default;
        bool operator==(const FilePath&) const = default;

    private:

        std::filesystem::path                  m_Path{};
    };

    class FileHandle
    {
        friend class FileSystem;

    public:

        bool IsValid() const { return m_HadOpen; }

        const std::vector<std::byte>* TryGetBinaryData() const { if (m_HadOpen && !m_Binary.empty()) { return &m_Binary; } return nullptr; }

    private:

        static FileHandle FailedToOpen();
        
    private:

        bool                                    m_HadOpen = false;
        std::vector<std::byte>                  m_Binary;
    };
    
    class FileSystem
    {
    public:

        static void Mount();

        static FileHandle Load(const FilePath& filePath);
        
        static FilePath ToAbsoluteEnginePath(const FilePath& filePath);

        static bool IsSanitized(const FilePath& path);
        static void Sanitize(FilePath& path);

    private:

        static void Sanitize(std::filesystem::path& path);
        
    private:

        static std::filesystem::path ToAbsoluteEnginePath(std::filesystem::path filePath);
        
    private:

        // Engine absolute mount path. Can be different on different machine or user environment
        static std::filesystem::path            m_EngineMountPath;
    };
}

template <>
struct std::hash<ZE::Core::FilePath>
{
    std::size_t operator()(const ZE::Core::FilePath& value) const noexcept
    {
        return value.GetHash();
    }
};

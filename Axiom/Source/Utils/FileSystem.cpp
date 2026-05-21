#include "axpch.h"

#include "FileSystem.h"

#include "Core/Log.h"

namespace Axiom {
    std::filesystem::path FileSystem::workingDirectory = std::filesystem::current_path();

    bool FileSystem::exists(const std::filesystem::path& filePath) {
        return std::filesystem::exists(filePath);
    }

    std::vector<uint8_t> FileSystem::readFile(const std::filesystem::path& filePath) {
        std::ifstream in{filePath, std::ios::binary};
        if (!in) {
            AX_CORE_LOG_ERROR("Could not open the file: {0}", filePath.string());
            return {};
        }

        std::vector<uint8_t> buf(std::istreambuf_iterator<char>(in), {});
        return buf;
    }

    std::string FileSystem::readFileStr(const std::filesystem::path& filePath) {
        std::ifstream in{filePath, std::ios::in | std::ios::binary | std::ios::ate};
        if (!in) {
            AX_CORE_LOG_ERROR("Could not open the file: {0}", filePath.string());
            return {};
        }

        const auto size = in.tellg();
        std::string buf(size, '\0');
        in.seekg(0, std::ios::beg);
        in.read(buf.data(), size);
        in.close();
        return buf;
    }

    void FileSystem::writeFile(const std::filesystem::path& filePath, std::vector<uint8_t>& data) {
        std::ofstream out{filePath, std::ios::binary | std::ios::trunc};
        if (!out)
            AX_CORE_LOG_ERROR("Could not write the file: {0}", filePath.string());
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
        out.close();
    }

    std::vector<FileInfo> FileSystem::getDirectory(const std::filesystem::path& folderPath) {
        std::vector<FileInfo> files{};
        if (!exists(folderPath)) {
            AX_CORE_LOG_WARN("Could not get the directory: {0}", folderPath.string());
            return files;
        }

        for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
            FileInfo fileInfo;
            fileInfo.name = entry.path().filename().string();
            fileInfo.isDirectory = entry.is_directory();
            fileInfo.size = entry.is_regular_file() ? entry.file_size() : 0;
            files.push_back(fileInfo);
        }
        return files;
    }

    void FileSystem::createDirectory(const std::filesystem::path& folderPath) {
        std::filesystem::create_directory(folderPath);
    }

    void FileSystem::setWorkingDirectory(const std::filesystem::path& folderPath) {
        if (std::filesystem::exists(folderPath) && std::filesystem::is_directory(folderPath)) {
            workingDirectory = std::filesystem::absolute(folderPath);
            std::filesystem::current_path(workingDirectory);
        } else {
            AX_CORE_LOG_ERROR("The specified path does not exist or is not a directory: {}", folderPath.string());
        }
    }
} // namespace Axiom

#include "filesystem.hpp"

#include <filesystem>
#include <fstream>

namespace tftp {
bool FileSystem::fileExists(const char *filename) const {
    return std::filesystem::exists(filename);
}

bool FileSystem::createFile(const char *filename) const {
    std::ofstream file(filename);
    return file.good();
}

bool FileSystem::deleteFile(const char *filename) const {
    return std::filesystem::remove(filename);
}

ssize_t FileSystem::readFile(const char *filename, char *buffer, ssize_t size,
                             ssize_t offset) const {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return -1;
    }
    file.seekg(offset);
    file.read(buffer, size);
    return file.gcount();
}

bool FileSystem::writeFile(const char *filename, const char *buffer,
                           ssize_t size, ssize_t offset) const {
    std::ofstream file(filename, std::ios::binary | std::ios::out);
    if (!file) {
        return false;
    }
    file.seekp(offset);
    file.write(buffer, size);
    return file.good();
}
}  // namespace tftp
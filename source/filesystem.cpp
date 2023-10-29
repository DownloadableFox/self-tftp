#include "filesystem.hpp"

#include <memory.h>

#include <filesystem>
#include <fstream>

#include "utils.hpp"

namespace tftp {
bool FileSystem::exists(const std::string filename) const {
    return std::filesystem::exists(filename);
}

bool FileSystem::create(const std::string filename) const {
    std::ofstream file(filename);
    file.close();
    return file.good();
}

bool FileSystem::remove(const std::string filename) const {
    return std::filesystem::remove(filename);
}

ssize_t FileSystem::read(const std::string filename, char *buffer, ssize_t size,
                         ssize_t offset) const {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return -1;

    file.seekg(offset);
    file.read(buffer, size);
    file.close();
    return file.gcount();
}

ssize_t FileSystem::write(const std::string filename, char *buffer,
                          ssize_t size, ssize_t offset) const {
    std::ofstream file(filename, std::ios::binary | std::ios::out);
    if (!file) return -1;

    file.seekp(offset);
    file.write(buffer, size);
    file.close();
    return file.good();
}

ssize_t FileSystem::append(const std::string filename, char *buffer,
                           ssize_t size) const {
    printBuffer(buffer, size);
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (!file) return -1;

    file.write(buffer, size);
    file.close();
    return file.good();
}
}  // namespace tftp
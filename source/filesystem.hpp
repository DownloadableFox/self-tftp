#pragma once

#include <sys/types.h>

#include <string>

namespace tftp {
class FileSystem {
   public:
    FileSystem() = default;
    ~FileSystem() = default;

    bool exists(const std::string filename) const;
    bool create(const std::string filename) const;
    bool remove(const std::string filename) const;

    ssize_t read(const std::string filename, char *buffer, ssize_t size,
                 ssize_t offset = 0) const;

    ssize_t write(const std::string filename, char *buffer, ssize_t size,
                  ssize_t offset = 0) const;

    ssize_t append(const std::string filename, char *buffer,
                   ssize_t size) const;
};
}  // namespace tftp
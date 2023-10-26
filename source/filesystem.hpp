#pragma once

#include <sys/types.h>

#include <string>

namespace tftp {
class FileSystem {
   public:
    FileSystem() = default;
    ~FileSystem() = default;

    bool fileExists(const char *filename) const;
    bool createFile(const char *filename) const;
    bool deleteFile(const char *filename) const;
    ssize_t readFile(const char *filename, char *buffer, ssize_t size,
                     ssize_t offset) const;
    bool writeFile(const char *filename, const char *buffer, ssize_t size,
                   ssize_t offset) const;
};
}  // namespace tftp
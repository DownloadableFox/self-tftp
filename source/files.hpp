#pragma once

#include <string>
#include <vector>

#include "common.hpp"
#include "filesystem.hpp"

namespace tftp {
enum class FileWorkerMode {
    NetAscii = 0,
    Octet = 1,
    Mail = 2,
};

class FileWorker {
   public:
    FileWorker(const std::string filename, const FileWorkerMode mode)
        : filename(filename), mode(mode) {}
    virtual ~FileWorker() {}

    virtual bool open() = 0;
    virtual bool close() = 0;
    virtual bool exists() = 0;
    virtual bool remove() = 0;

    virtual ssize_t read(char *dst, ssize_t size, ssize_t offset = 0) = 0;
    virtual ssize_t write(char *src, ssize_t size, ssize_t offset = 0) = 0;
    virtual ssize_t append(char *src, ssize_t size) = 0;

   protected:
    const std::string filename;
    const FileWorkerMode mode;
};

class FileWorkerFactory {
   public:
    virtual FileWorker *create(std::string filename, FileWorkerMode mode) = 0;
};

// Buffered file worker
class BufferedFileWorker : public FileWorker {
   private:
    std::vector<char> buffer;
    ssize_t buffer_position = 0;

    const FileSystem &filesystem;

   public:
    BufferedFileWorker(const std::string filename, const FileWorkerMode mode,
                       const unsigned int buffer_size,
                       const FileSystem &filesystem)
        : FileWorker(filename, mode), filesystem(filesystem) {
        this->buffer.reserve(buffer_size);
    }
    virtual ~BufferedFileWorker() { this->close(); }

    virtual bool open();
    virtual bool close();
    virtual bool exists();
    virtual bool remove();

    virtual ssize_t read(char *dst, ssize_t size, ssize_t offset = 0);
    virtual ssize_t write(char *src, ssize_t size, ssize_t offset = 0);
    virtual ssize_t append(char *src, ssize_t size);
};

class BufferedFileWorkerFactory : public FileWorkerFactory {
   private:
    const unsigned int buffer_size;
    const FileSystem &filesystem;

   public:
    BufferedFileWorkerFactory(const unsigned int buffer_size,
                              const FileSystem &filesystem)
        : buffer_size(buffer_size), filesystem(filesystem) {}
    ~BufferedFileWorkerFactory() {}

    virtual FileWorker *create(std::string filename, FileWorkerMode mode) {
        return new BufferedFileWorker(filename, mode, this->buffer_size,
                                      this->filesystem);
    }
};
}  // namespace tftp
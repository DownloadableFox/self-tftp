#include "files.hpp"

#include <arpa/inet.h>

#include <algorithm>

namespace tftp {
bool BufferedFileWorker::open() {
    // Check if the file exists
    if (!this->filesystem.exists(this->filename)) {
        // Create the file
        this->filesystem.create(this->filename);
    }

    return true;
}

bool BufferedFileWorker::close() {
    if (this->buffer_position > 0) {
        // Write the buffer to the file
        this->filesystem.append(this->filename, this->buffer.data(),
                                this->buffer_position);

        // Clear the buffer
        this->buffer_position = 0;
    }

    return true;
}

bool BufferedFileWorker::exists() {
    return this->filesystem.exists(this->filename);
}

bool BufferedFileWorker::remove() {
    return this->filesystem.remove(this->filename);
}

ssize_t BufferedFileWorker::read(char *dst, ssize_t size, ssize_t offset) {
    // Read from the file
    return this->filesystem.read(this->filename, dst, size, offset);
}

ssize_t BufferedFileWorker::write(char *src, ssize_t size, ssize_t offset) {
    return this->filesystem.write(this->filename, src, size, offset);
}

ssize_t BufferedFileWorker::append(char *src, ssize_t size) {
    // Check if the buffer is full
    if (this->buffer_position + size > this->buffer.capacity()) {
        // Write the buffer to the file
        this->filesystem.append(this->filename, this->buffer.data(),
                                this->buffer_position);

        // Clear the buffer
        this->buffer_position = 0;
        this->buffer.clear();
    }

    // Write to the buffer
    std::copy(src, src + size, this->buffer.begin() + this->buffer_position);
    this->buffer_position += size;

    return size;
}
}  // namespace tftp
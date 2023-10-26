#pragma once

#include <sys/types.h>

#include "filesystem.hpp"
#include "packets.hpp"

namespace tftp {
class AbstractController {
   public:
    virtual ssize_t handlePacket(char *src, char *dst, ssize_t size) = 0;
};

class ControllerState {
   public:
    enum State {
        IDLE,
        READING,
        WRITING,
    } state;

    char filename[512];
    uint16_t block_number;
    ReadWriteRequestMode mode;

    char file_buffer[1024 * 1024 * 5]; // 5MB

    ControllerState() : state(State::IDLE), block_number(0) {}

    void reset() {
        this->state = State::IDLE;
        this->block_number = 0;

        clearBuffer();
    }

    void setFilename(const char *filename) { strcpy(this->filename, filename); }
    void setMode(ReadWriteRequestMode mode) { this->mode = mode; }
    void incrementBlockNumber() { ++this->block_number; }
    void setState(State state) { this->state = state; }

    // File buffer
    void clearBuffer() { memset(this->file_buffer, 0, sizeof(this->file_buffer)); }
    void bufferData(const char *data, ssize_t size, ssize_t offset) {
        memcpy(this->file_buffer + offset, data, size);
    }
};

class Controller : public AbstractController {
   public:
    virtual ssize_t handlePacket(char *src, char *dst, ssize_t size);

    Controller(FileSystem &filesystem) : filesystem(filesystem) {}

   private:
    // State
    ControllerState state;
    FileSystem &filesystem;

    // Packet handlers
    ssize_t handleReadRequestPacket(char *src, char *dst, ssize_t size);
    ssize_t handleWriteRequestPacket(char *src, char *dst, ssize_t size);
    ssize_t handleDataPacket(char *src, char *dst, ssize_t size);
    ssize_t handleAckPacket(char *src, char *dst, ssize_t size);

    // Utility functions
    PacketType getPacketType(const char *src) const;
    ssize_t writeError(char *dst, ErrorCode error_code,
                       const char *message) const;
    ssize_t sendNextBlock(char *dst);
};
}  // namespace tftp
#pragma once

#include <sys/types.h>

#include "files.hpp"
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

    uint16_t block_number = 0;
    FileWorker *file_worker = nullptr;

    ControllerState() { this->reset(); }

    void reset() {
        this->state = State::IDLE;
        this->block_number = 0;

        // Delete file worker
        if (this->file_worker != nullptr) delete this->file_worker;
        this->file_worker = nullptr;
    }

    // Getters and setters
    void incrementBlockNumber() { ++this->block_number; }
    uint16_t getBlockNumber() const { return this->block_number; }

    void setState(State state) { this->state = state; }
    State getState() const { return this->state; }

    void setFileWorker(FileWorker *file_worker) {
        if (this->file_worker != nullptr) {
            delete this->file_worker;
        }

        this->file_worker = file_worker;
    }
};

class Controller : public AbstractController {
   public:
    virtual ssize_t handlePacket(char *src, char *dst, ssize_t size);

    Controller(FileWorkerFactory &worker_factory)
        : worker_factory(worker_factory) {}

   private:
    // State
    ControllerState state;
    FileWorkerFactory &worker_factory;

    // Packet handlers
    ssize_t handleReadRequestPacket(char *src, char *dst, ssize_t size);
    ssize_t handleWriteRequestPacket(char *src, char *dst, ssize_t size);
    ssize_t handleDataPacket(char *src, char *dst, ssize_t size);
    ssize_t handleAckPacket(char *src, char *dst, ssize_t size);

    // Utility functions
    PacketType getPacketType(const char *src) const;
    bool openFileWorker(const char *filename, ReadWriteRequestMode mode);
    bool openFileWorker(const char *filename, FileWorkerMode mode);

    // Reply functions
    ssize_t sendNextBlock(char *dst);
    ssize_t sendError(char *dst, ErrorCode error_code,
                      const char *message) const;
};
}  // namespace tftp

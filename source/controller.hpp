#pragma once

#include "common.hpp"
#include "files.hpp"
#include "packets.hpp"

constexpr uint16_t DEFAULT_WINDOW_SIZE = 512;
constexpr uint16_t MAX_WINDOW_SIZE = 65460;
constexpr uint16_t MIN_WINDOW_SIZE = 8;
constexpr int DEFAULT_TIMEOUT_MS = 5000;

namespace tftp {
class PacketHandler {
   public:
    virtual ssize_t handlePacket(char *src, char *dst, ssize_t src_size) = 0;
};

class ControllerContext {
   public:
    enum State {
        IDLE,
        READING,
        WRITING,
    } state;

    // TFTP Operation state
    uint16_t block_number = 0;
    time_t last_packet_time = 0;
    bool is_last_block = false;

    // TFTP Options
    uint16_t window_size = DEFAULT_WINDOW_SIZE;
    int timeout_ms = DEFAULT_TIMEOUT_MS;

    FileWorker *file_worker = nullptr;

    ControllerContext() { this->reset(); }

    void reset() {
        // Reset state
        this->state = State::IDLE;
        this->block_number = 0;
        this->last_packet_time = 0;
        this->is_last_block = false;

        // Reset options
        this->window_size = DEFAULT_WINDOW_SIZE;
        this->timeout_ms = DEFAULT_TIMEOUT_MS;

        // Delete file worker
        if (this->file_worker != nullptr) delete this->file_worker;
        this->file_worker = nullptr;
    }

    // Getters and setters
    void incrementBlockNumber() { ++this->block_number; }
    uint16_t getBlockNumber() const { return this->block_number; }

    void setState(State state) { this->state = state; }
    State getState() const { return this->state; }

    void setLastPacketTime(time_t last_packet_time) {
        this->last_packet_time = last_packet_time;
    }

    time_t getLastPacketTime() const { return this->last_packet_time; }

    void setLastBlock(bool is_last_block) {
        this->is_last_block = is_last_block;
    }

    bool isLastBlock() const { return this->is_last_block; }

    void setWindowSize(uint16_t window_size) {
        this->window_size = window_size;
    }

    uint16_t getWindowSize() const { return this->window_size; }

    void setTimeoutMs(int timeout_ms) { this->timeout_ms = timeout_ms; }
    int getTimeoutMs() const { return this->timeout_ms; }

    void setFileWorker(FileWorker *file_worker) {
        if (this->file_worker != nullptr) {
            delete this->file_worker;
        }

        this->file_worker = file_worker;
    }
};

class Controller : public PacketHandler {
   public:
    virtual ssize_t handlePacket(char *src, char *dst, ssize_t src_size);

    Controller(FileWorkerFactory &worker_factory)
        : worker_factory(worker_factory) {}

   private:
    // State
    ControllerContext state;
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

#include "controller.hpp"

#include <arpa/inet.h>
#include <memory.h>

#include <iostream>

namespace tftp {
ssize_t Controller::handlePacket(char *src, char *dst, ssize_t src_size) {
    if (src_size < 4) {
        return this->sendError(dst, ErrorCode::NOT_DEFINED,
                               "Packet too small!");
    }

    try {
        const PacketType type = this->getPacketType(src);

        switch (type) {
            case PacketType::RRQ:
                return this->handleReadRequestPacket(src, dst, src_size);
            case PacketType::WRQ:
                return this->handleWriteRequestPacket(src, dst, src_size);
            case PacketType::DATA:
                return this->handleDataPacket(src, dst, src_size);
            case PacketType::ACK:
                return this->handleAckPacket(src, dst, src_size);
            default:
                throw std::runtime_error("Invalid packet type!");
        }
    } catch (const std::exception &e) {
        return this->sendError(dst, ErrorCode::NOT_DEFINED, e.what());
    }
}

ssize_t Controller::handleReadRequestPacket(char *src, char *dst,
                                            ssize_t src_size) {
    // Check if we are already reading or writing
    if (this->state.getState() != ControllerState::State::IDLE) {
        return this->sendError(dst, ErrorCode::NOT_DEFINED,
                               "Server is currently busy!");
    }

    // Deserialize packet
    ReadRequestPacket packet;
    packet.deserialize(src);

    // Reset state
    this->state.reset();

    // Check if file exists
    if (!this->openFileWorker(packet.filename, packet.mode)) {
        return this->sendError(dst, ErrorCode::FILE_NOT_FOUND,
                               "File does not exist!");
    }

    // Set state
    this->state.setState(ControllerState::State::READING);
    this->state.incrementBlockNumber();

    // Send first block
    return this->sendNextBlock(dst);
}

ssize_t Controller::handleWriteRequestPacket(char *src, char *dst,
                                             ssize_t src_size) {
    // Check if we are already reading or writing
    if (this->state.getState() != ControllerState::State::IDLE) {
        return this->sendError(dst, ErrorCode::NOT_DEFINED,
                               "Server is currently busy!");
    }

    // Deserialize packet
    WriteRequestPacket packet;
    packet.deserialize(src);

    // Reset state
    this->state.reset();

    // Overwrite file if it exists
    if (this->openFileWorker(packet.filename, packet.mode)) {
        this->state.file_worker->remove();
    }

    // Set state
    this->state.setState(ControllerState::State::WRITING);
    this->state.incrementBlockNumber();

    // Send ack packet
    AckPacket ack_packet(0);
    return ack_packet.serialize(dst);
}

ssize_t Controller::handleDataPacket(char *src, char *dst, ssize_t src_size) {
    // Check if we are already reading or writing
    if (this->state.getState() != ControllerState::State::WRITING) {
        return this->sendError(dst, ErrorCode::NOT_DEFINED, "Invalid state!");
    }

    // Deserialize packet
    DataPacket packet;
    packet.data_size = src_size - 4;
    packet.deserialize(src);

    // Check if the block number is correct
    if (packet.block_number != this->state.block_number) {
        return this->sendError(dst, ErrorCode::NOT_DEFINED,
                               "Invalid block number!");
    }

    // Increment block number
    this->state.incrementBlockNumber();

    // Write data to filebuffer.
    this->state.file_worker->append(packet.data, packet.data_size);

    // Reset state if we read less than 512 bytes
    if (packet.data_size < 512) {
        this->state.reset();
    }

    // Send ack packet
    AckPacket ack_packet(packet.block_number);
    return ack_packet.serialize(dst);
}

ssize_t Controller::handleAckPacket(char *src, char *dst, ssize_t src_size) {
    if (this->state.state != ControllerState::State::READING) {
        return -1;
    }

    // Deserialize packet
    AckPacket packet;
    packet.deserialize(src);

    // Check if the block number is correct
    if (packet.block_number == this->state.block_number) {
        this->state.incrementBlockNumber();
    } else if (packet.block_number > this->state.block_number) {
        return this->sendError(dst, ErrorCode::NOT_DEFINED,
                               "Invalid block number!");
    }

    // Send next block
    return this->sendNextBlock(dst);
}

// Utility functions
PacketType Controller::getPacketType(const char *src) const {
    return static_cast<PacketType>(
        ntohs(*reinterpret_cast<const uint16_t *>(src)));
}

bool Controller::openFileWorker(const char *filename,
                                ReadWriteRequestMode mode) {
    // Translating ReadWriteRequestMode to FileWorkerMode
    FileWorkerMode file_worker_mode = static_cast<FileWorkerMode>(mode);
    return this->openFileWorker(filename, file_worker_mode);
}

bool Controller::openFileWorker(const char *filename, FileWorkerMode mode) {
    // Create file worker
    FileWorker *file_worker = this->worker_factory.create(filename, mode);
    this->state.setFileWorker(file_worker);

    return file_worker->exists();
}

// Reply functions
ssize_t Controller::sendError(char *dst, ErrorCode error_code,
                              const char *message) const {
    // Create error packet
    ErrorPacket packet(error_code, message);
    return packet.serialize(dst);
}

ssize_t Controller::sendNextBlock(char *dst) {
    // Calculate offset
    ssize_t offset = (this->state.block_number - 1) * 512;

    // Read from file
    static char buffer[512];
    ssize_t bytes_read = this->state.file_worker->read(buffer, 512, offset);

    // Check if we reached the end of the file
    if (bytes_read < 0) {
        return this->sendError(dst, ErrorCode::NOT_DEFINED,
                               "Failed to read file!");
    }

    // Create data packet
    DataPacket data_packet(this->state.block_number, buffer, bytes_read);

    // Reset state if we read less than 512 bytes
    if (bytes_read < 512) {
        this->state.reset();
    }

    return data_packet.serialize(dst);
}
}  // namespace tftp
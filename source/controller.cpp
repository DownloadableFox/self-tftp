#include "controller.hpp"

#include <arpa/inet.h>
#include <memory.h>

#include <iostream>

namespace tftp {
ssize_t Controller::handlePacket(char *src, char *dst, ssize_t src_size) {
    if (src_size < 2) {
        return this->writeError(dst, ErrorCode::NOT_DEFINED,
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
        return this->writeError(dst, ErrorCode::NOT_DEFINED, e.what());
    }
}

ssize_t Controller::handleReadRequestPacket(char *src, char *dst,
                                            ssize_t src_size) {
    if (src_size < 4) {
        return this->writeError(dst, ErrorCode::NOT_DEFINED,
                                "Packet too small!");
    }

    // Check if we are already reading or writing
    if (this->state.state != ControllerState::State::IDLE) {
        return this->writeError(dst, ErrorCode::NOT_DEFINED,
                                "Server is currently busy!");
    }

    // Deserialize packet
    ReadRequestPacket packet;
    packet.deserialize(src);

    // Print packet information
    std::cout << "-> ReadRequestPacket: " << packet.filename << " "
              << (int)packet.mode << std::endl;

    // Check if file exists
    if (!this->filesystem.fileExists(packet.filename)) {
        return this->writeError(dst, ErrorCode::FILE_NOT_FOUND,
                                "File does not exist!");
    }

    // Set state
    this->state.reset();
    this->state.setState(ControllerState::State::READING);
    this->state.setFilename(packet.filename);
    this->state.setMode(packet.mode);
    this->state.incrementBlockNumber();

    // Send first block
    return sendNextBlock(dst);
}

ssize_t Controller::handleWriteRequestPacket(char *src, char *dst,
                                             ssize_t src_size) {
    if (src_size < 4) {
        return this->writeError(dst, ErrorCode::NOT_DEFINED,
                                "Packet too small!");
    }

    // Check if we are already reading or writing
    if (this->state.state != ControllerState::State::IDLE) {
        return this->writeError(dst, ErrorCode::NOT_DEFINED,
                                "Server is currently busy!");
    }

    // Deserialize packet
    WriteRequestPacket packet;
    packet.deserialize(src);

    // Print packet information
    std::cout << "-> WriteRequestPacket: " << packet.filename << " "
              << (int)packet.mode << std::endl;

    // Set state
    this->state.reset();
    this->state.setState(ControllerState::State::WRITING);
    this->state.setFilename(packet.filename);
    this->state.setMode(packet.mode);
    this->state.incrementBlockNumber();

    // Send ack packet
    AckPacket ack_packet(0);
    return ack_packet.serialize(dst);
}

ssize_t Controller::handleDataPacket(char *src, char *dst, ssize_t src_size) {
    if (src_size < 4) {
        return this->writeError(dst, ErrorCode::NOT_DEFINED,
                                "Packet too small!");
    }

    // Check if we are already reading or writing
    if (this->state.state != ControllerState::State::WRITING) {
        return this->writeError(dst, ErrorCode::NOT_DEFINED, "Invalid state!");
    }

    // Deserialize packet
    DataPacket packet;
    packet.data_size = src_size - 4;
    packet.deserialize(src);

    // Print packet information
    std::cout << "-> DataPacket: " << packet.block_number << std::endl;

    // Write data to file
    ssize_t bytes_written = this->filesystem.writeFile(
        this->state.filename, packet.data, packet.data_size,
        (packet.block_number - 1) * 512);

    if (bytes_written < 0) {
        return this->writeError(dst, ErrorCode::NOT_DEFINED,
                                "Failed to write file!");
    }

    // Reset state if we read less than 512 bytes
    if (packet.data_size < 512) {
        this->state.reset();
    }

    // Send ack packet
    AckPacket ack_packet(packet.block_number);
    return ack_packet.serialize(dst);
}

ssize_t Controller::handleAckPacket(char *src, char *dst, ssize_t src_size) {
    if (src_size < 4) {
        return this->writeError(dst, ErrorCode::NOT_DEFINED,
                                "Packet too small!");
    }

    if (this->state.state != ControllerState::State::READING) {
        return -1;
    }

    AckPacket packet;
    packet.deserialize(src);

    std::cout << "-> AckPacket: " << packet.block_number << std::endl;

    if (packet.block_number == this->state.block_number) {
        this->state.incrementBlockNumber();
    } else if (packet.block_number > this->state.block_number) {
        return this->writeError(dst, ErrorCode::NOT_DEFINED,
                                "Invalid block number!");
    }

    return sendNextBlock(dst);
}

PacketType Controller::getPacketType(const char *src) const {
    return static_cast<PacketType>(
        ntohs(*reinterpret_cast<const uint16_t *>(src)));
}

ssize_t Controller::writeError(char *dst, ErrorCode error_code,
                               const char *message) const {
    std::cout << "-> Error: " << message << std::endl;

    ErrorPacket packet(error_code, message);
    return packet.serialize(dst);
}

ssize_t Controller::sendNextBlock(char *dst) {
    std::cout << "-> Sending block #" << this->state.block_number << std::endl;

    char buffer[512];
    ssize_t offset = (this->state.block_number - 1) * 512;

    ssize_t bytes_read =
        this->filesystem.readFile(this->state.filename, buffer, 512, offset);

    if (bytes_read < 0) {
        return this->writeError(dst, ErrorCode::NOT_DEFINED,
                                "Failed to read file!");
    }

    // Create data packet
    DataPacket data_packet(this->state.block_number, buffer, bytes_read);

    // Print packet information
    std::cout << "-> DataPacket: " << data_packet.block_number << std::endl;

    // Reset state if we read less than 512 bytes
    if (bytes_read < 512) {
        this->state.reset();
    }

    return data_packet.serialize(dst);
}
}  // namespace tftp
#include "controller.hpp"

#include <memory.h>

#include <iostream>

namespace tftp {
ssize_t Controller::handlePacket(char *src, char *dst, ssize_t src_size) {
    if (src_size < 4) {
        return this->sendError(dst, ErrorCode::ILLEGAL_OPERATION,
                               "Packet too small!");
    }

    try {
        const PacketType type = this->getPacketType(src);

        // Check if we are already reading or writing
        if (this->state.getLastPacketTime() != 0) {
            time_t current_time = time(nullptr);
            time_t time_diference =
                current_time - this->state.getLastPacketTime();

            if (time_diference > this->state.getTimeoutMs()) {
                this->state.reset();

                if (type == PacketType::ACK || type == PacketType::DATA ||
                    type == PacketType::OACK) {
                    return this->sendError(
                        dst, ErrorCode::NOT_DEFINED,
                        "A timeout has occured in the request!");
                }
            }
        }

        // Set last packet time
        this->state.setLastPacketTime(time(nullptr));

        // Handle packet
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
    if (this->state.getState() != ControllerContext::State::IDLE) {
        return this->sendError(dst, ErrorCode::NOT_DEFINED,
                               "Server is currently busy!");
    }

    // Deserialize packet
    ReadRequestPacket packet;
    ssize_t bytes_read = packet.deserialize(src);

    // Deserialize options
    if (bytes_read < src_size) {
        packet.deserializeOptions(src + bytes_read, src_size - bytes_read);
    }

    // Reset state
    this->state.reset();

    // Check if file exists
    if (!this->openFileWorker(packet.filename, packet.mode)) {
        return this->sendError(dst, ErrorCode::FILE_NOT_FOUND,
                               "File does not exist!");
    }

    // Set state
    this->state.setState(ControllerContext::State::READING);
    this->state.incrementBlockNumber();

    // Check and apply options
    if (packet.options.size() > 0) {
        // Create OACK packet
        OptionAckPacket oack_packet;

        // Check if we have a window size option
        auto window_size_option = packet.options.find("blksize");
        if (window_size_option != packet.options.end()) {
            auto value = std::stoi(window_size_option->second);

            if (value >= 8 && value <= 65464) {
                this->state.setWindowSize(value);
                oack_packet.options["blksize"] = window_size_option->second;
            }
        }

        // Check if we have a timeout option
        auto timeout_option = packet.options.find("timeout");
        if (timeout_option != packet.options.end()) {
            this->state.setTimeoutMs(std::stoi(timeout_option->second));
            oack_packet.options["timeout"] = timeout_option->second;
        }

        // Send OACK packet
        return oack_packet.serialize(dst);
    }

    // Send first block
    return this->sendNextBlock(dst);
}

ssize_t Controller::handleWriteRequestPacket(char *src, char *dst,
                                             ssize_t src_size) {
    // Check if we are already reading or writing
    if (this->state.getState() != ControllerContext::State::IDLE) {
        return this->sendError(dst, ErrorCode::NOT_DEFINED,
                               "Server is currently busy!");
    }

    // Deserialize packet
    WriteRequestPacket packet;
    ssize_t bytes_read = packet.deserialize(src);

    // Deserialize options
    if (bytes_read < src_size) {
        packet.deserializeOptions(src + bytes_read, src_size - bytes_read);
    }

    // Reset state
    this->state.reset();

    // Overwrite file if it exists
    if (this->openFileWorker(packet.filename, packet.mode)) {
        this->state.file_worker->remove();
    }

    // Set state
    this->state.setState(ControllerContext::State::WRITING);
    this->state.incrementBlockNumber();

    // Check and apply options
    if (packet.options.size() > 0) {
        // Create OACK packet
        OptionAckPacket oack_packet;

        // Check if we have a window size option
        auto window_size_option = packet.options.find("blksize");
        if (window_size_option != packet.options.end()) {
            auto value = std::stoi(window_size_option->second);

            if (value >= 8 && value <= 65464) {
                this->state.setWindowSize(value);
                oack_packet.options["blksize"] = window_size_option->second;
            }
        }

        // Check if we have a timeout option
        auto timeout_option = packet.options.find("timeout");
        if (timeout_option != packet.options.end()) {
            this->state.setTimeoutMs(std::stoi(timeout_option->second));
            oack_packet.options["timeout"] = timeout_option->second;
        }

        // Send OACK packet
        return oack_packet.serialize(dst);
    }

    // Send ack packet
    AckPacket ack_packet(0);
    return ack_packet.serialize(dst);
}

ssize_t Controller::handleDataPacket(char *src, char *dst, ssize_t src_size) {
    // Check if we are already reading or writing
    if (this->state.getState() != ControllerContext::State::WRITING) {
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
    if (packet.data_size < this->state.window_size) {
        this->state.reset();
    }

    // Send ack packet
    AckPacket ack_packet(packet.block_number);
    return ack_packet.serialize(dst);
}

ssize_t Controller::handleAckPacket(char *src, char *dst, ssize_t src_size) {
    if (this->state.state != ControllerContext::State::READING) {
        return -1;
    }

    // Deserialize packet
    AckPacket packet;
    packet.deserialize(src);

    // Check if the block number is correct
    if (packet.block_number == this->state.block_number) {
        this->state.incrementBlockNumber();

        // Check if we reached the end of the file
        if (this->state.isLastBlock()) {
            this->state.reset();
            return -1;
        }
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
    ssize_t offset = (this->state.block_number - 1) * this->state.window_size;

    // Read from file
    char *buffer = new char[this->state.window_size + 4];
    ssize_t bytes_read =
        this->state.file_worker->read(buffer, this->state.window_size, offset);

    // Check if we reached the end of the file
    if (bytes_read < 0) {
        return this->sendError(dst, ErrorCode::NOT_DEFINED,
                               "Failed to read file!");
    }

    // Create data packet
    DataPacket data_packet(this->state.block_number, buffer, bytes_read);

    // Reset state if we read less than the window size
    if (bytes_read < this->state.window_size) {
        this->state.is_last_block = true;
    }

    return data_packet.serialize(dst);
}
}  // namespace tftp
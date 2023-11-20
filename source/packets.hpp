#pragma once

#include <memory.h>
#include <sys/types.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>

namespace tftp {
enum class PacketType : uint16_t {
    UNKNOWN = 0,
    RRQ = 1,
    WRQ = 2,
    DATA = 3,
    ACK = 4,
    ERROR = 5,
    OACK = 6,
};

class Packet {
   public:
    PacketType type;  // 2 bytes

    Packet() : type(PacketType::UNKNOWN) {}
    Packet(PacketType type) : type(type) {}
    virtual ~Packet() = default;

    virtual ssize_t serialize(char *dst) const = 0;
    virtual ssize_t deserialize(const char *src) = 0;
};

enum ReadWriteRequestMode : uint8_t {
    NETASCII = 0,
    OCTET = 1,
    MAIL = 2,
};

class ReadWriteRequestPacket : public Packet {
   protected:
    ReadWriteRequestPacket() : Packet(PacketType::UNKNOWN) {}
    ReadWriteRequestPacket(const char *filename, const char *mode);
    ~ReadWriteRequestPacket() = default;

   public:
    char filename[512];
    ReadWriteRequestMode mode;
    std::map<std::string, std::string> options;

    ssize_t serialize(char *dst) const override;
    ssize_t deserialize(const char *src) override;
    void deserializeOptions(const char *src, ssize_t size);
};

class ReadRequestPacket : public ReadWriteRequestPacket {
   public:
    ReadRequestPacket() : ReadWriteRequestPacket() { type = PacketType::RRQ; }
    ReadRequestPacket(const char *filename, const char *mode)
        : ReadWriteRequestPacket(filename, mode) {
        type = PacketType::RRQ;
    }
    ~ReadRequestPacket() = default;
};

class WriteRequestPacket : public ReadWriteRequestPacket {
   public:
    WriteRequestPacket() : ReadWriteRequestPacket() { type = PacketType::WRQ; }
    WriteRequestPacket(const char *filename, const char *mode)
        : ReadWriteRequestPacket(filename, mode) {
        type = PacketType::WRQ;
    }
    ~WriteRequestPacket() = default;
};

class DataPacket : public Packet {
   public:
    uint16_t block_number;
    ssize_t data_size;
    char data[UINT16_MAX - 4];

    DataPacket() : Packet(PacketType::DATA) {}
    DataPacket(uint16_t block_number, const char *data, ssize_t data_size)
        : Packet(PacketType::DATA),
          block_number(block_number),
          data_size(data_size) {
        memcpy(this->data, data, data_size);
    }
    ~DataPacket() = default;

    ssize_t serialize(char *dst) const override;
    ssize_t deserialize(const char *src) override;
};

class AckPacket : public Packet {
   public:
    uint16_t block_number;

    AckPacket() : Packet(PacketType::ACK) {}
    AckPacket(uint16_t block_number)
        : Packet(PacketType::ACK), block_number(block_number) {}
    ~AckPacket() = default;

    ssize_t serialize(char *dst) const override;
    ssize_t deserialize(const char *src) override;
};

enum class ErrorCode : uint16_t {
    NOT_DEFINED = 0,
    FILE_NOT_FOUND = 1,
    ACCESS_VIOLATION = 2,
    DISK_FULL = 3,
    ILLEGAL_OPERATION = 4,
    UNKNOWN_TRANSFER_ID = 5,
    FILE_ALREADY_EXISTS = 6,
    NO_SUCH_USER = 7,
};

class ErrorPacket : public Packet {
   public:
    ErrorCode code;
    char message[512];

    ErrorPacket() : Packet(PacketType::ERROR) {}
    ErrorPacket(ErrorCode error_code, const char *error_message)
        : Packet(PacketType::ERROR), code(error_code) {
        strcpy(this->message, error_message);
    }
    ~ErrorPacket() = default;

    ssize_t serialize(char *dst) const override;
    ssize_t deserialize(const char *src) override;
};

class OptionAckPacket : public Packet {
   public:
    std::map<std::string, std::string> options;
    OptionAckPacket() : Packet(PacketType::OACK) {}
    OptionAckPacket(std::map<std::string, std::string> options)
        : Packet(PacketType::OACK), options(options) {}
    ~OptionAckPacket() = default;

    ssize_t serialize(char *dst) const override;
    ssize_t deserialize(const char *src) override;
};
}  // namespace tftp

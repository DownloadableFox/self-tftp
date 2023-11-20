#include "packets.hpp"

#include <stdexcept>

#include "common.hpp"

namespace tftp {
ssize_t ReadWriteRequestPacket::serialize(char *dst) const {
    // Create packet with size
    ssize_t size = 0;

    // Write opcode
    *reinterpret_cast<uint16_t *>(dst) = htons(static_cast<uint16_t>(type));
    size += sizeof(type);

    // Write filename
    strcpy(dst + size, filename);
    size += strlen(filename) + 1;

    // Write mode
    switch (mode) {
        case ReadWriteRequestMode::NETASCII:
            strcpy(dst + size, "netascii");
            break;
        case ReadWriteRequestMode::OCTET:
            strcpy(dst + size, "octet");
            break;
        case ReadWriteRequestMode::MAIL:
            strcpy(dst + size, "mail");
            break;
        default:
            throw std::runtime_error("Invalid file read/write mode!");
    }
    size += strlen(dst + size) + 1;

    // Write options
    for (auto &option : options) {
        // Write option name
        strcpy(dst + size, option.first.c_str());
        size += strlen(option.first.c_str()) + 1;

        // Write option value
        strcpy(dst + size, option.second.c_str());
        size += strlen(option.second.c_str()) + 1;
    }

    printf("<- Read/Write request packet: { filename: %s, mode: %s }\n",
           filename, dst + size);

    return size;
}

ssize_t ReadWriteRequestPacket::deserialize(const char *src) {
    // Read packet with size
    ssize_t size = 0;

    // Read opcode
    type = static_cast<PacketType>(
        ntohs(*reinterpret_cast<const uint16_t *>(src)));
    size += sizeof(type);

    // Read filename
    strcpy(filename, src + size);
    size += strlen(filename) + 1;

    // Read mode
    char mode_str[512];
    strcpy(mode_str, src + size);
    size += strlen(mode_str) + 1;

    // Parse mode
    for (char *c = mode_str; *c; ++c) {
        *c = std::tolower(*c);
    }

    if (strcmp(mode_str, "netascii") == 0) {
        mode = ReadWriteRequestMode::NETASCII;
    } else if (strcmp(mode_str, "octet") == 0) {
        mode = ReadWriteRequestMode::OCTET;
    } else if (strcmp(mode_str, "mail") == 0) {
        mode = ReadWriteRequestMode::MAIL;
    } else {
        throw std::runtime_error("Invalid file read/write mode!");
    }

    printf("-> Read/Write request packet: { filename: %s, mode: %s }\n",
           filename, mode_str);

    return size;
}

void ReadWriteRequestPacket::deserializeOptions(const char *src,
                                                ssize_t src_size) {
    // Read options
    ssize_t size = 0;

    while (size < src_size) {
        // Read option name
        std::string option_name(src + size);
        size += strlen(src + size) + 1;

        // Read option value
        std::string option_value(src + size);
        size += strlen(src + size) + 1;

        // Add option
        options[option_name] = option_value;
    }

    // Print the parsed options
    printf("-> Read/Write request packet (options): { options: %ld }\n",
           options.size());

    for (auto &option : options) {
        std::cout << "\t" << option.first << ": " << option.second << std::endl;
    }
}

ssize_t DataPacket::serialize(char *dst) const {
    // Create packet with size
    ssize_t size = 0;

    // Write opcode
    *reinterpret_cast<uint16_t *>(dst) = htons(static_cast<uint16_t>(type));
    size += sizeof(type);

    // Write block number
    *reinterpret_cast<uint16_t *>(dst + size) = htons(block_number);
    size += sizeof(block_number);

    // Write data
    memcpy(dst + size, data, data_size);
    size += data_size;

    printf("<- Data packet: { block_number: %d, data_size: %ld }\n",
           block_number, data_size);
    return size;
}

ssize_t DataPacket::deserialize(const char *src) {
    // Read packet with size
    ssize_t size = 0;

    // Read opcode
    type = static_cast<PacketType>(
        ntohs(*reinterpret_cast<const uint16_t *>(src)));
    size += sizeof(type);

    // Read block number
    block_number = ntohs(*reinterpret_cast<const uint16_t *>(src + size));
    size += sizeof(block_number);

    // Read data
    memcpy(data, src + size, data_size);
    size += data_size;

    printf("-> Data packet: { block_number: %d, data_size: %ld }\n",
           block_number, data_size);

    return size;
}

ssize_t AckPacket::serialize(char *dst) const {
    // Create packet with size
    ssize_t size = 0;

    // Write opcode
    *reinterpret_cast<uint16_t *>(dst) = htons(static_cast<uint16_t>(type));
    size += sizeof(type);

    // Write block number
    *reinterpret_cast<uint16_t *>(dst + size) = htons(block_number);
    size += sizeof(block_number);

    printf("<- Ack packet: { block_number: %d }\n", block_number);
    return size;
}

ssize_t AckPacket::deserialize(const char *src) {
    // Read packet with size
    ssize_t size = 0;

    // Read opcode
    type = static_cast<PacketType>(
        ntohs(*reinterpret_cast<const uint16_t *>(src)));
    size += sizeof(type);

    // Read block number
    block_number = ntohs(*reinterpret_cast<const uint16_t *>(src + size));
    size += sizeof(block_number);

    printf("-> Ack packet: { block_number: %d }\n", block_number);

    return size;
}

ssize_t ErrorPacket::serialize(char *dst) const {
    // Create packet with size
    ssize_t size = 0;

    // Write opcode
    *reinterpret_cast<uint16_t *>(dst) = htons(static_cast<uint16_t>(type));
    size += sizeof(type);

    // Write error code
    *reinterpret_cast<uint16_t *>(dst + size) =
        htons(static_cast<uint16_t>(code));
    size += sizeof(code);

    // Write error message
    strcpy(dst + size, message);
    size += strlen(message) + 1;

    printf("<- Error packet: { code: %d, message: %s }\n", (int)code, message);
    return size;
}

ssize_t ErrorPacket::deserialize(const char *src) {
    // Read packet with size
    ssize_t size = 0;

    // Read opcode
    type = static_cast<PacketType>(
        ntohs(*reinterpret_cast<const uint16_t *>(src)));
    size += sizeof(type);

    // Read error code
    code = static_cast<ErrorCode>(
        ntohs(*reinterpret_cast<const uint16_t *>(src + size)));
    size += sizeof(code);

    // Read error message
    strcpy(message, src + size);
    size += strlen(message) + 1;

    printf("-> Error packet: { code: %d, message: %s }\n", (int)code, message);

    return size;
}

ssize_t OptionAckPacket::serialize(char *dst) const {
    // Create packet with size
    ssize_t size = 0;

    // Write opcode
    *reinterpret_cast<uint16_t *>(dst) = htons(static_cast<uint16_t>(type));
    size += sizeof(type);

    // Write options
    for (auto &option : options) {
        // Write option name
        strcpy(dst + size, option.first.c_str());
        size += strlen(option.first.c_str()) + 1;

        // Write option value
        strcpy(dst + size, option.second.c_str());
        size += strlen(option.second.c_str()) + 1;
    }

    printf("<- Option ack packet: { options: %ld }\n", options.size());
    return size;
}

ssize_t OptionAckPacket::deserialize(const char *src) {
    // Read packet with size
    ssize_t size = 0;

    // Read opcode
    type = static_cast<PacketType>(
        ntohs(*reinterpret_cast<const uint16_t *>(src)));
    size += sizeof(type);

    // Read options
    while (size < 512) {
        // Read option name
        std::string option_name(src + size);
        size += strlen(src + size) + 1;

        // Read option value
        std::string option_value(src + size);
        size += strlen(src + size) + 1;

        // Add option
        options[option_name] = option_value;
    }

    printf("-> Option ack packet: { options: %ld }\n", options.size());

    return size;
}
}  // namespace tftp
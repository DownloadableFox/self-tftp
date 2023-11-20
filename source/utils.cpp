#include "utils.hpp"

#include <memory.h>

#include <sstream>

namespace tftp {
void printBuffer(char *buffer, ssize_t size) {
    for (ssize_t i = 0; i < size; i += 8) {
        for (ssize_t j = 0; j < 8 && j + i < size; j++) {
            char c = buffer[i + j];

            // Check if the character is printable
            if (c < 32 || c > 126) c = '.';
            printf("%c ", c);
        }

        printf(" | ");

        for (ssize_t j = 0; j < 8 && j + i < size; j++) {
            printf("%02x ", (unsigned char)buffer[i + j]);
        }

        printf("\n");
    }

    printf("\n");
}
};  // namespace tftp
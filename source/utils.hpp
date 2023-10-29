#pragma once

#include <sys/types.h>

namespace tftp {
void printBuffer(char *buffer, ssize_t size);
//void octectToNetASCII(char *buffer, ssize_t size);
//void netASCIIToOctect(char *buffer, ssize_t size);
};
#pragma once

#ifndef _WIN32

// Linux
#include <arpa/inet.h>
#include <sys/types.h>

#else

// Windows
#include <winsock2.h>
#undef ERROR  // winsock2.h defines ERROR as 0

// Windows does not have ssize_t
#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#endif

#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>

#include "controller.hpp"

namespace tftp {
constexpr inline unsigned int BUFFER_SIZE = 2048;

class Server {
   public:
    Server(std::string ip, unsigned int port, AbstractController& controller);
    ~Server();

    void listen();

   private:
    unsigned int socket_fd;
    struct sockaddr_in server_addr, client_addr;
    char request[BUFFER_SIZE], response[BUFFER_SIZE];

    AbstractController& controller;
};
}  // namespace tftp

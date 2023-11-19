#include "server.hpp"

#include <iostream>
#include <stdexcept>

#include "common.hpp"

namespace tftp {
Server::Server(std::string ip, unsigned int port,
               AbstractController &controller)
    : controller(controller) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        throw std::runtime_error("Failed to initialize socket library");
    }
#endif

    this->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (!this->socket_fd) {
        throw std::runtime_error("Failed to create socket");
    }

    this->server_addr.sin_family = AF_INET;
    this->server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    this->server_addr.sin_port = htons(port);
}

Server::~Server() {
#ifdef _WIN32
    closesocket(this->socket_fd);
    WSACleanup();
#else
    close(this->socket_fd);
#endif
}

void Server::listen() {
    std::cout << "Listening on " << inet_ntoa(this->server_addr.sin_addr) << ":"
              << ntohs(this->server_addr.sin_port) << std::endl;

    if (bind(this->socket_fd, (struct sockaddr *)&this->server_addr,
             sizeof(this->server_addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    for (;;) {
        // For safety reasons, we clear the buffer on each request
        memset(this->request, 0, BUFFER_SIZE);
        memset(this->response, 0, BUFFER_SIZE);

// Attempt to receive data
#ifdef _WIN32
        int client_len = sizeof(this->client_addr);

#else
        socklen_t client_len = sizeof(this->client_addr);
#endif
        ssize_t request_size =
            recvfrom(this->socket_fd, this->request, BUFFER_SIZE, 0,
                     (struct sockaddr *)&this->client_addr, &client_len);

        if (request_size < 0) {
            throw std::runtime_error("Failed to receive data");
        }

        std::cout << "Received " << request_size << " bytes from "
                  << inet_ntoa(this->client_addr.sin_addr) << ":"
                  << ntohs(this->client_addr.sin_port) << std::endl;

        // Handle the request
        ssize_t response_size = this->controller.handlePacket(
            this->request, this->response, request_size);

        // Skip packets that don't require a response
        if (response_size <= 0) {
            continue;
        }

        // Send the response
        ssize_t bytes_sent = sendto(
            this->socket_fd, this->response, response_size, 0,
            (struct sockaddr *)&this->client_addr, sizeof(this->client_addr));

        if (bytes_sent < 0) {
            throw std::runtime_error("Failed to send data");
        }

        std::cout << "Sent " << response_size << " bytes to "
                  << inet_ntoa(this->client_addr.sin_addr) << ":"
                  << ntohs(this->client_addr.sin_port) << std::endl;
    }
}
}  // namespace tftp

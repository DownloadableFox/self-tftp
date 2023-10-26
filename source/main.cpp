#include <iostream>

#include "server.hpp"

void usage(const char *name) {
    std::cerr << "Usage: " << name << " [port]" << std::endl;
    exit(1);
}

int main(int argc, char **argv) {
    // Default port
    unsigned int port = 69;

    // Check if the user wants to see the help message
    if (argc > 2 || (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help"))) {
        usage(argv[0]);
    }

    // Parse the port number
    try {
        if (argc == 2) port = std::stoi(argv[1]);
    } catch (const std::exception &e) {
        std::cerr << "Invalid port number!" << std::endl;
        usage(argv[0]);
    }

    // Create a filesystem
    tftp::FileSystem filesystem;

    // Create a controller for incomming connections
    tftp::Controller controller(filesystem);

    // Create a server that listens on all interfaces on port 8080
    tftp::Server server("0.0.0.0", port, controller);

    try {
        server.listen();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
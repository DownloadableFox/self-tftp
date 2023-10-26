#include <iostream>

#include "server.hpp"

int main() {
    // Create a filesystem
    tftp::FileSystem filesystem;

    // Create a controller for incomming connections
    tftp::Controller controller(filesystem);

    // Create a server that listens on all interfaces on port 8080
    tftp::Server server("0.0.0.0", 8080, controller);

    try {
        server.listen();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
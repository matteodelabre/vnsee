#include "client.hpp"
#include "screen.hpp"
#include <iostream>

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Missing server IP address.\n";
        return EXIT_FAILURE;
    }

    rm::screen screen;
    client client_instance{/* ip = */ argv[1], /* port = */ 5900, screen};

    client_instance.start();
    return EXIT_SUCCESS;
}

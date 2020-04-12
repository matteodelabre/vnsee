#include "app/client.hpp"
#include "rmioc/screen.hpp"
#include "rmioc/touch.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept> // IWYU pragma: keep
// IWYU pragma: no_include <bits/exception.h>
#include <string>

auto main(int argc, const char* argv[]) -> int
{
    std::string server_ip;
    constexpr int server_port = 5900;

    if (argc < 2)
    {
        // Guess the server IP from the SSH client IP
        const char* ssh_conn = getenv("SSH_CONNECTION");

        if (ssh_conn == nullptr)
        {
            std::cerr << "No server IP given and no active SSH session.\n";
            std::cerr << "Please specify the VNC server IP.\n";
            return EXIT_FAILURE;
        }

        // Extract the remote client IP from the first field
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const char* ssh_conn_end = ssh_conn + std::strlen(ssh_conn);
        const char* remote_ip_end = std::find(ssh_conn, ssh_conn_end, ' ');

        // Remove IPv4-mapped IPv6 prefix
        const char* remote_ip_start = ssh_conn;
        auto ipv4_prefix = "::ffff:";

        if (std::strncmp(ipv4_prefix, ssh_conn, std::strlen(ipv4_prefix)) == 0)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            remote_ip_start += std::strlen(ipv4_prefix);
        }

        server_ip = std::string(remote_ip_start, remote_ip_end);
    }
    else
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        server_ip = argv[1];
    }

    try
    {
        rmioc::screen screen;
        rmioc::touch touch;

        std::cerr << "Connecting to "
            << server_ip << ":" << server_port << "...\n";

        app::client client{
            server_ip.data(), server_port,
            screen, touch
        };

        std::cerr << "\e[1A\e[KConnected to "
            << server_ip << ':' << server_port << "!\n";

        client.event_loop();
    }
    catch (const std::exception& err)
    {
        std::cerr << "Error: " << err.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

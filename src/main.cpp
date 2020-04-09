#include "client.hpp"
#include "rmioc/input.hpp"
#include "rmioc/screen.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept> // IWYU pragma: keep
// IWYU pragma: no_include <bits/exception.h>
#include <string>

int main(int argc, const char* argv[])
{
    std::string server_ip;
    int server_port = 5900;

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

        // The first component is the remote client IP, extract it
        std::size_t ssh_conn_len = std::strlen(ssh_conn);
        std::size_t remote_ip_end = 0;

        while (remote_ip_end < ssh_conn_len && ssh_conn[remote_ip_end] != ' ')
        {
            ++remote_ip_end;
        }

        // Remove IPv4-mapped IPv6 prefix
        std::size_t remote_ip_start = 0;
        auto ipv4_prefix = "::ffff:";

        if (std::strncmp(ipv4_prefix, ssh_conn, std::strlen(ipv4_prefix)) == 0)
        {
            remote_ip_start = std::strlen(ipv4_prefix);
        }

        server_ip = std::string(
            ssh_conn + remote_ip_start,
            ssh_conn + remote_ip_end
        );
    }
    else
    {
        server_ip = argv[1];
    }

    try
    {
        rmioc::screen screen;
        rmioc::input input;

        std::cerr << "Connecting to "
            << server_ip << ":" << server_port << "...\n";

        client client_instance{
            server_ip.data(), server_port,
            screen, input
        };

        std::cerr << "\e[1A\e[KConnected to "
            << server_ip << ':' << server_port << "!\n";

        client_instance.event_loop();
    }
    catch (const std::exception& err)
    {
        std::cerr << "Error: " << err.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

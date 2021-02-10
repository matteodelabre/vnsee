#include "options.hpp"
#include "app/client.hpp"
#include "config.hpp"
#include "rmioc/device.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept> // IWYU pragma: keep
// IWYU pragma: no_include <bits/exception.h>
#include <string>
#include <utility>
#include <vector>

/**
 * Print a short help message with usage information.
 *
 * @param name Name of the current executable file.
 */
auto help(const char* name) -> void
{
    std::cout << "Usage: " << name << " [IP [PORT]] [OPTION...]\n"
"Connect to the VNC server at IP:PORT.\n\n"
"Only when launching " PROJECT_NAME " from a SSH session is the IP optional,\n"
"in which case the client’s IP address is taken by default.\n"
"By default, PORT is 5900.\n\n"
"Available options:\n"
"  -h, --help           Show this help message and exit.\n"
"  -v, --version        Show the current version of " PROJECT_NAME " and exit.\n"
"  --no-buttons         Disable buttons interaction.\n"
"  --no-pen             Disable pen interaction.\n"
"  --no-touch           Disable touchscreen interaction.\n";
}

/**
 * Print current version.
 */
auto version() -> void
{
    std::cout << PROJECT_NAME << ' ' << PROJECT_VERSION << '\n';
}

constexpr int default_server_port = 5900;
constexpr int min_port = 1;
constexpr int max_port = (1U << 16U) - 1;

auto main(int argc, const char* argv[]) -> int
{
    // Read options from the command line
    std::string server_ip;
    int server_port = default_server_port;
    rmioc::device_request request(rmioc::device_request::screen);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char* const name = argv[0];
    auto [opts, oper] = options::parse(argv + 1, argv + argc);

    if ((opts.count("help") >= 1) || (opts.count("h") >= 1))
    {
        help(name);
        return EXIT_SUCCESS;
    }

    if ((opts.count("version") >= 1) || (opts.count("v") >= 1))
    {
        version();
        return EXIT_SUCCESS;
    }

    if (oper.size() > 2)
    {
        std::cerr << "Too many operands: at most 2 are needed, you gave "
            << oper.size() << ".\n"
            "Run “" << name << " --help” for more information.\n";
        return EXIT_FAILURE;
    }

    if (!oper.empty())
    {
        server_ip = oper[0];
    }
    else
    {
        // Guess the server IP from the SSH client IP
        const char* ssh_conn = getenv("SSH_CONNECTION");

        if (ssh_conn == nullptr)
        {
            std::cerr << "No server IP given and no active SSH session.\n"
                "Please specify the VNC server IP.\n"
                "Run “" << name << " --help” for more information.\n";
            return EXIT_FAILURE;
        }

        // Extract the remote client IP from the first field
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const char* ssh_conn_end = ssh_conn + std::strlen(ssh_conn);
        const char* remote_ip_end = std::find(ssh_conn, ssh_conn_end, ' ');

        // Remove IPv4-mapped IPv6 prefix
        const char* remote_ip_start = ssh_conn;
        const auto *ipv4_prefix = "::ffff:";

        if (std::strncmp(ipv4_prefix, ssh_conn, std::strlen(ipv4_prefix)) == 0)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            remote_ip_start += std::strlen(ipv4_prefix);
        }

        server_ip = std::string(remote_ip_start, remote_ip_end);
    }

    if (oper.size() == 2)
    {
        try
        {
            server_port = std::stoi(oper[1]);
        }
        catch (const std::invalid_argument&)
        {
            std::cerr << "“" << oper[1]
                << "” is not a valid port number.\n";
            return EXIT_FAILURE;
        }

        if (server_port < min_port || server_port > max_port)
        {
            std::cerr << server_port << " is not a valid port "
                "number. Valid values range from " << min_port << " to "
                << max_port << ".\n";
            return EXIT_FAILURE;
        }
    }

    if (opts.count("no-buttons") >= 1)
    {
        opts.erase("no-buttons");
    }
    else
    {
        request.set_buttons(true);
    }

    if (opts.count("no-pen") >= 1)
    {
        opts.erase("no-pen");
    }
    else
    {
        request.set_pen(true);
    }

    if (opts.count("no-touch") >= 1)
    {
        opts.erase("no-touch");
    }
    else
    {
        request.set_touch(true);
    }

    if (!opts.empty())
    {
        std::cerr << "Unknown options: ";

        for (
            auto opt_it = std::cbegin(opts);
            opt_it != std::cend(opts);
            ++opt_it
        )
        {
            std::cerr << opt_it->first;

            if (std::next(opt_it) != std::cend(opts))
            {
                std::cerr << ", ";
            }
        }

        std::cerr << "\n";
        return EXIT_FAILURE;
    }

    // Start the client
    try
    {
        rmioc::device device = rmioc::device::detect(request);

        std::cerr << "Connecting to "
            << server_ip << ":" << server_port << "...\n";

        app::client client{server_ip.data(), server_port, device};

        std::cerr << "\e[1A\e[KConnected to "
            << server_ip << ':' << server_port << "!\n";

        if (!client.event_loop())
        {
            std::cerr << "Connection closed by the server.\n";
            return EXIT_FAILURE;
        }
    }
    catch (const std::exception& err)
    {
        std::cerr << "Error: " << err.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

#include "options.hpp"
#include "app/client.hpp"
#include "rmioc/buttons.hpp"
#include "rmioc/pen.hpp"
#include "rmioc/screen.hpp"
#include "rmioc/touch.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
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
    std::cerr << "Usage: " << name << " [IP [PORT]] [OPTION...]\n"
"Connect to the VNC server at IP:PORT.\n\n"
"Only when launching VNSee from a SSH session is the IP optional,\n"
"in which case the client’s IP address is taken by default.\n"
"By default, PORT is 5900.\n\n"
"Available options:\n"
"  -h, --help               show this help message\n"
"  --no-buttons             disable buttons interaction\n"
"  --no-pen                 disable pen interaction\n"
"  --no-touch               disable touchscreen interaction\n";
}

constexpr int default_server_port = 5900;
constexpr int min_port = 1;
constexpr int max_port = (1U << 16U) - 1;

auto main(int argc, const char* argv[]) -> int
{
    // Read options from the command line
    std::string server_ip;
    int server_port = default_server_port;
    bool enable_buttons = true;
    bool enable_pen = true;
    bool enable_touch = true;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char* const name = argv[0];
    auto [opts, oper] = options::parse(argv + 1, argv + argc);

    if ((opts.count("help") >= 1) || (opts.count("h") >= 1))
    {
        help(name);
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
        enable_buttons = false;
        opts.erase("no-buttons");
    }

    if (opts.count("no-pen") >= 1)
    {
        enable_pen = false;
        opts.erase("no-pen");
    }

    if (opts.count("no-touch") >= 1)
    {
        enable_touch = false;
        opts.erase("no-touch");
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
        rmioc::screen screen;
        std::unique_ptr<rmioc::buttons> buttons;
        std::unique_ptr<rmioc::pen> pen;
        std::unique_ptr<rmioc::touch> touch;

        if (enable_buttons)
        {
            buttons = std::make_unique<rmioc::buttons>();
        }

        if (enable_pen)
        {
            pen = std::make_unique<rmioc::pen>();
        }

        if (enable_touch)
        {
            touch = std::make_unique<rmioc::touch>();
        }

        std::cerr << "Connecting to "
            << server_ip << ":" << server_port << "...\n";

        app::client client{
            server_ip.data(), server_port,
            screen, buttons.get(), pen.get(), touch.get()
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

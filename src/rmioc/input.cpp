#include "input.hpp"
#include <cerrno>
#include <string>
#include <system_error>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <poll.h>
#include <unistd.h>

namespace rmioc
{

input::input(const char* device_path)
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-signed-bitwise)
: input_fd(open(device_path, O_RDONLY | O_NONBLOCK))
{
    if (this->input_fd == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::input) Open input device " + std::string(device_path)
        );
    }
}

input::~input()
{
    close(this->input_fd);
}

void input::setup_poll(pollfd& in_pollfd)
{
    in_pollfd.fd = this->input_fd;
    in_pollfd.events = POLLIN;
}

auto input::fetch_events() -> std::vector<input_event>
{
    std::vector<input_event> result;
    input_event current_event{};

    while (read(this->input_fd, &current_event, sizeof(current_event)) != -1)
    {
        if (current_event.type == EV_SYN)
        {
            std::swap(this->queued_events, result);
            return result;
        }

        this->queued_events.push_back(current_event);
    }

    if (errno != EAGAIN)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::touch::fetch_events) Read touchscreen event"
        );
    }

    return result;
}

} // namespace rmioc

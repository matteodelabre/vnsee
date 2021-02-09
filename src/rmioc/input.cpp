#include "input.hpp"
#include <cerrno>
#include <string>
#include <system_error>
#include <utility>
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
    if (this->input_fd != -1)
    {
        close(this->input_fd);
    }
}

input::input(input&& other) noexcept
: input_fd(std::exchange(other.input_fd, -1))
, queued_events(std::move(other.queued_events))
{}

auto input::operator=(input&& other) noexcept -> input&
{
    if (this->input_fd != -1)
    {
        close(this->input_fd);
    }

    this->input_fd = std::exchange(other.input_fd, -1);
    this->queued_events = std::move(other.queued_events);
    return *this;
}

void input::setup_poll(pollfd& in_pollfd) const
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

auto input::get_axis_limits(unsigned int type) const -> std::pair<int, int>
{
    input_absinfo result{};

    // NOLINTNEXTLINE(hicpp-signed-bitwise): Use of C library
    if (ioctl(this->input_fd, EVIOCGABS(type), &result) == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::input) Get axis state"
        );
    }

    return std::make_pair(result.minimum, result.maximum);
}

} // namespace rmioc

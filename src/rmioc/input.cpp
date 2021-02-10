#include "input.hpp"
#include <array>
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

constexpr std::size_t read_events_batch_size = 64;

auto input::fetch_events() -> std::vector<input_event>
{
    std::vector<input_event> result;
    std::array<input_event, read_events_batch_size> read_events{};

    constexpr auto one_bytes = sizeof(input_event);
    constexpr auto max_bytes = read_events_batch_size * one_bytes;
    ssize_t maybe_read_bytes = 0;

    while ((maybe_read_bytes = read(
        this->input_fd,
        read_events.data(),
        max_bytes
    )) != -1)
    {
        auto read_bytes = static_cast<std::size_t>(maybe_read_bytes);

        if (read_bytes < one_bytes)
        {
            throw std::runtime_error(
                "Invalid read of " + std::to_string(read_bytes) + " bytes, "
                "less than the size of an input struct (expected at least "
                + std::to_string(one_bytes) + " bytes)"
            );
        }

        std::size_t read_size = read_bytes / one_bytes;
        this->queued_events.reserve(this->queued_events.size() + read_size);

        for (std::size_t i = 0; i < read_size; ++i)
        {
            if (read_events.at(i).type == EV_SYN)
            {
                std::swap(this->queued_events, result);
                return result;
            }

            this->queued_events.emplace_back(read_events.at(i));
        }
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

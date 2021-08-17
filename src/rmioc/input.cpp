#include "input.hpp"
#include <array>
#include <cerrno>
#include <iosfwd>
#include <string>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace rmioc
{

constexpr auto bits_per_byte = 8U;

template<size_t Size>
inline auto get_bit(const std::array<uint8_t, Size>& bitset, size_t index)
-> bool
{
    return bitset.at(index / bits_per_byte) & (1U << (index % bits_per_byte));
}

auto supported_input_events(int input_fd) -> input_events
{
    std::array<uint8_t, EV_MAX / bits_per_byte + 1> evbitset{};

    if (
        ioctl(
            input_fd,
            // NOLINTNEXTLINE(hicpp-signed-bitwise)
            EVIOCGBIT(0, sizeof(evbitset.size())),
            evbitset.data()
        ) == -1
    )
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::supported_input_events) Get supported events"
        );
    }

    input_events result;
    result.set_syn(get_bit(evbitset, EV_SYN));
    result.set_key(get_bit(evbitset, EV_KEY));
    result.set_rel(get_bit(evbitset, EV_REL));
    result.set_abs(get_bit(evbitset, EV_ABS));
    return result;
}

auto supported_key_types(int input_fd) -> key_types
{
    std::array<uint8_t, KEY_MAX / bits_per_byte + 1> keybitset{};

    // Get the bit fields of available keys.
    if (
        ioctl(
            input_fd,
            // NOLINTNEXTLINE(hicpp-signed-bitwise)
            EVIOCGBIT(EV_KEY, keybitset.size()),
            keybitset.data()
        ) == -1
    )
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::supported_key_types) Get supported keys"
        );
    }

    key_types result;
    result.set_power(get_bit(keybitset, KEY_POWER));
    result.set_tool_pen(get_bit(keybitset, BTN_TOOL_PEN));
    result.set_tool_rubber(get_bit(keybitset, BTN_TOOL_RUBBER));
    return result;
}

auto supported_abs_types(int input_fd) -> abs_types
{
    std::array<uint8_t, ABS_MAX / bits_per_byte + 1> absbitset{};

    // Get the bit fields of available keys.
    if (
        ioctl(
            input_fd,
            // NOLINTNEXTLINE(hicpp-signed-bitwise)
            EVIOCGBIT(EV_ABS, absbitset.size()),
            absbitset.data()
        ) == -1
    )
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::supported_abs_types) Get supported axes"
        );
    }

    abs_types result;
    result.set_x(get_bit(absbitset, ABS_X));
    result.set_y(get_bit(absbitset, ABS_Y));
    result.set_pressure(get_bit(absbitset, ABS_PRESSURE));
    result.set_distance(get_bit(absbitset, ABS_DISTANCE));
    result.set_tilt_x(get_bit(absbitset, ABS_TILT_X));
    result.set_tilt_y(get_bit(absbitset, ABS_TILT_Y));
    result.set_mt_slot(get_bit(absbitset, ABS_MT_SLOT));
    result.set_mt_tracking_id(get_bit(absbitset, ABS_MT_TRACKING_ID));
    result.set_mt_position_x(get_bit(absbitset, ABS_MT_POSITION_X));
    result.set_mt_position_y(get_bit(absbitset, ABS_MT_POSITION_Y));
    result.set_mt_pressure(get_bit(absbitset, ABS_MT_PRESSURE));
    result.set_mt_orientation(get_bit(absbitset, ABS_MT_ORIENTATION));
    return result;
}

input::input(const char* device_path)
// NOLINTNEXTLINE(hicpp-signed-bitwise)
: input_fd(device_path, O_RDONLY | O_NONBLOCK)
{
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

    // NOLINTNEXTLINE(hicpp-signed-bitwise)
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

#include "screen_rm2fb.hpp"
#include "mxcfb.hpp"
#include <cerrno>
#include <cstdint>
#include <system_error>
#include <utility>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <unistd.h>

namespace rm2fb
{

enum class message_types : long
{
    init = 1,
    update,
    xochitl
};

struct update_data
{
    message_types message_type;
    mxcfb::update_data update;
};

}

namespace rmioc
{

constexpr auto screen_width = 1404;
constexpr auto screen_height = 1872;
constexpr auto screen_depth = 2;
constexpr auto screen_mem_len = screen_width * screen_height * screen_depth;

screen_rm2fb::screen_rm2fb(const char* shm_path, int msgqueue_key)
{
    this->framebuf_fd = shm_open(shm_path, O_RDWR, 0);

    if (this->framebuf_fd == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::screen_rm2fb) Open shared memory framebuffer"
        );
    }

    this->msgqueue_id = msgget(msgqueue_key, 0);

    if (this->msgqueue_id == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::screen_rm2fb) Open framebuffer message queue"
        );
    }

    void* mmap_res = mmap(
        /* addr = */ nullptr,
        /* len = */ screen_mem_len,
        // NOLINTNEXTLINE(hicpp-signed-bitwise): Use of C library
        /* prot = */ PROT_READ | PROT_WRITE,
        /* flags = */ MAP_SHARED,
        /* fd = */ this->framebuf_fd,
        /* __offset = */ 0
    );

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    if (mmap_res == MAP_FAILED)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::screen_rm2fb) Map framebuffer to memory"
        );
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): Use of C library
    this->framebuf_ptr = reinterpret_cast<uint8_t*>(mmap_res);
}

screen_rm2fb::~screen_rm2fb()
{
    if (this->framebuf_ptr != nullptr)
    {
        munmap(this->framebuf_ptr, screen_mem_len);
    }

    if (this->framebuf_fd != -1)
    {
        close(this->framebuf_fd);
    }
}

screen_rm2fb::screen_rm2fb(screen_rm2fb&& other) noexcept
: framebuf_fd(std::exchange(other.framebuf_fd, -1))
, msgqueue_id(std::exchange(other.msgqueue_id, -1))
, framebuf_ptr(std::exchange(other.framebuf_ptr, nullptr))
{}

auto screen_rm2fb::operator=(screen_rm2fb&& other) noexcept -> screen_rm2fb&
{
    if (this->framebuf_ptr != nullptr)
    {
        munmap(this->framebuf_ptr, screen_mem_len);
    }

    if (this->framebuf_fd != -1)
    {
        close(this->framebuf_fd);
    }

    this->framebuf_fd = std::exchange(other.framebuf_fd, -1);
    this->msgqueue_id = std::exchange(other.msgqueue_id, -1);
    this->framebuf_ptr = std::exchange(other.framebuf_ptr, nullptr);
    return *this;
}

void screen_rm2fb::update(
    int x, int y, int w, int h, waveform_modes mode, bool wait)
{
    mxcfb::update_data update{};
    update.update_region = mxcfb::rect::clip(
        x, y, w, h,
        this->get_xres(), this->get_yres()
    );
    update.waveform_mode = mode;
    update.temp = mxcfb::temps::normal;
    update.update_mode = mxcfb::update_modes::partial;
    update.flags = 0;

    this->send_update(update, wait);
}

void screen_rm2fb::update(waveform_modes mode, bool wait)
{
    mxcfb::update_data update{};
    update.update_region = mxcfb::rect::clip(
        0, 0, this->get_xres(), this->get_yres(),
        this->get_xres(), this->get_yres()
    );
    update.waveform_mode = mode;
    update.temp = mxcfb::temps::normal;
    update.update_mode = mxcfb::update_modes::full;
    update.flags = 0;

    this->send_update(update, wait);
}

void screen_rm2fb::send_update(mxcfb::update_data& update, bool /*wait*/) const
{
    if (!update.update_region)
    {
        return;
    }

    rm2fb::update_data message{};
    message.message_type = rm2fb::message_types::update;
    message.update = update;

    if (msgsnd(this->msgqueue_id, &message, sizeof(message), 0) == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::screen_rm2fb::send_update) Screen update"
        );
    }
}

auto screen_rm2fb::get_data() -> std::uint8_t*
{
    return this->framebuf_ptr;
}

auto screen_rm2fb::get_xres() const -> int
{
    return screen_width;
}

auto screen_rm2fb::get_xres_memory() const -> int
{
    return screen_width;
}

auto screen_rm2fb::get_yres() const -> int
{
    return screen_height;
}

auto screen_rm2fb::get_yres_memory() const -> int
{
    return screen_height;
}

auto screen_rm2fb::get_bits_per_pixel() const -> unsigned short
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    return screen_depth * 8;
}

auto screen_rm2fb::get_red_format() const -> component_format
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    return component_format{/* offset = */ 11, /* length = */ 5};
}

auto screen_rm2fb::get_green_format() const -> component_format
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    return component_format{/* offset = */ 5, /* length = */ 6};
}

auto screen_rm2fb::get_blue_format() const -> component_format
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    return component_format{/* offset = */ 0, /* length = */ 5};
}

} // namespace rmioc

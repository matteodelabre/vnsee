#include "screen.hpp"
#include "mxcfb.hpp"
#include <cerrno>
#include <cstdint>
#include <system_error>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace rmioc
{

screen::screen()
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): Use of C library
: framebuf_fd(open("/dev/fb0", O_RDWR))
{
    if (this->framebuf_fd == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::screen) Open device framebuffer"
        );
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): Use of C library
    if (ioctl(
            this->framebuf_fd,
            FBIOGET_VSCREENINFO,
            &this->framebuf_varinfo
        ) == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::screen) Fetch framebuffer vscreeninfo"
        );
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): Use of C library
    if (ioctl(
            this->framebuf_fd,
            FBIOGET_FSCREENINFO,
            &this->framebuf_fixinfo
        ) == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::screen) Fetch framebuffer fscreeninfo"
        );
    }

    // ↓ Use of C library
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    this->framebuf_ptr = reinterpret_cast<uint8_t*>(mmap(
        /* addr = */ nullptr,
        /* len = */ this->framebuf_fixinfo.smem_len,
        // NOLINTNEXTLINE(hicpp-signed-bitwise): Use of C library
        /* prot = */ PROT_READ | PROT_WRITE,
        /* flags = */ MAP_SHARED,
        /* fd = */ this->framebuf_fd,
        /* __offset = */ 0
    ));
}

screen::~screen()
{
    munmap(this->framebuf_ptr, this->framebuf_fixinfo.smem_len);
    this->framebuf_ptr = nullptr;

    close(this->framebuf_fd);
    this->framebuf_fd = -1;
}

void screen::update(
    int x, int y, int w, int h, mxcfb::waveform_modes mode, bool wait)
{
    // Clip update region to screen bounds
    if (x < 0)
    {
        w += x;
        x = 0;
    }

    if (y < 0)
    {
        h += y;
        y = 0;
    }

    int xres = static_cast<int>(this->get_xres());
    int yres = static_cast<int>(this->get_yres());

    if (x + w > xres)
    {
        w = xres - x;
    }

    if (y + h > yres)
    {
        h = yres - y;
    }

    // Ignore out of bounds or null updates
    if (x > xres || y > yres || w == 0 || h == 0)
    {
        return;
    }

    mxcfb::update_data update{};
    update.update_region.left = x;
    update.update_region.top = y;
    update.update_region.width = w;
    update.update_region.height = h;
    update.waveform_mode = mode;
    update.temp = mxcfb::temps::normal;
    update.update_mode = mxcfb::update_modes::partial;
    update.flags = 0;

    this->send_update(update, wait);
}

void screen::update(mxcfb::waveform_modes mode, bool wait)
{
    mxcfb::update_data update{};
    update.update_region.left = 0;
    update.update_region.top = 0;
    update.update_region.width = this->get_xres();
    update.update_region.height = this->get_yres();
    update.waveform_mode = mode;
    update.temp = mxcfb::temps::normal;
    update.update_mode = mxcfb::update_modes::full;
    update.flags = 0;

    this->send_update(update, wait);
}

void screen::send_update(mxcfb::update_data& update, bool wait)
{
    update.update_marker = this->next_update_marker;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): Use of C library
    if (ioctl(this->framebuf_fd, mxcfb::send_update, &update) == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::screen::send_update) Screen update"
        );
    }

    if (!wait)
    {
        return;
    }

    mxcfb::update_marker_data data{};
    data.update_marker = this->next_update_marker;
    data.collision_test = 0;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): Use of C library
    if (ioctl(this->framebuf_fd, mxcfb::wait_for_update_complete, &data) == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::screen::send_update) Wait for update completion"
        );
    }

    if (this->next_update_marker == rmioc::screen::max_update_marker)
    {
        this->next_update_marker = 1;
    }
    else
    {
        ++this->next_update_marker;
    }
}

auto screen::get_data() -> std::uint8_t*
{
    return this->framebuf_ptr;
}

auto screen::get_xres() const -> std::uint32_t
{
    return this->framebuf_varinfo.xres;
}

auto screen::get_xres_memory() const -> std::uint32_t
{
    return this->framebuf_varinfo.xres_virtual;
}

auto screen::get_yres() const -> std::uint32_t
{
    return this->framebuf_varinfo.yres;
}

auto screen::get_yres_memory() const -> std::uint32_t
{
    return this->framebuf_varinfo.yres_virtual;
}

auto screen::get_bits_per_pixel() const -> std::uint32_t
{
    return this->framebuf_varinfo.bits_per_pixel;
}

auto screen::get_red_offset() const -> std::uint32_t
{
    return this->framebuf_varinfo.red.offset;
}

auto screen::get_red_length() const -> std::uint32_t
{
    return this->framebuf_varinfo.red.length;
}

auto screen::get_red_max() const -> std::uint32_t
{
    return (1U << this->framebuf_varinfo.red.length) - 1;
}

auto screen::get_green_offset() const -> std::uint32_t
{
    return this->framebuf_varinfo.green.offset;
}

auto screen::get_green_length() const -> std::uint32_t
{
    return this->framebuf_varinfo.green.length;
}

auto screen::get_green_max() const -> std::uint32_t
{
    return (1U << this->framebuf_varinfo.green.length) - 1;
}

auto screen::get_blue_offset() const -> std::uint32_t
{
    return this->framebuf_varinfo.blue.offset;
}

auto screen::get_blue_length() const -> std::uint32_t
{
    return this->framebuf_varinfo.blue.length;
}

auto screen::get_blue_max() const -> std::uint32_t
{
    return (1U << this->framebuf_varinfo.blue.length) - 1;
}

} // namespace rmioc

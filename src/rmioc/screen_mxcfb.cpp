#include "screen_mxcfb.hpp"
#include "mxcfb.hpp"
#include <cerrno>
#include <cstdint>
#include <system_error>
#include <utility>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace rmioc
{

auto component_format::max() const -> std::uint32_t
{
    return (1U << this->length) - 1;
}

screen_mxcfb::screen_mxcfb(const char* device_path)
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): Use of C library
: framebuf_fd(open(device_path, O_RDWR))
{
    if (this->framebuf_fd == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::screen_mxcfb) Open device framebuffer"
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
            "(rmioc::screen_mxcfb) Fetch framebuffer vscreeninfo"
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
            "(rmioc::screen_mxcfb) Fetch framebuffer fscreeninfo"
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

screen_mxcfb::~screen_mxcfb()
{
    if (this->framebuf_ptr != nullptr)
    {
        munmap(this->framebuf_ptr, this->framebuf_fixinfo.smem_len);
    }

    if (this->framebuf_fd != -1)
    {
        close(this->framebuf_fd);
    }
}

screen_mxcfb::screen_mxcfb(screen_mxcfb&& other) noexcept
: framebuf_fd(std::exchange(other.framebuf_fd, -1))
, framebuf_varinfo(other.framebuf_varinfo)
, framebuf_fixinfo(other.framebuf_fixinfo)
, framebuf_ptr(std::exchange(other.framebuf_ptr, nullptr))
, next_update_marker(other.next_update_marker)
{}

auto screen_mxcfb::operator=(screen_mxcfb&& other) noexcept -> screen_mxcfb&
{
    if (this->framebuf_ptr != nullptr)
    {
        munmap(this->framebuf_ptr, this->framebuf_fixinfo.smem_len);
    }

    if (this->framebuf_fd != -1)
    {
        close(this->framebuf_fd);
    }

    this->framebuf_fd = std::exchange(other.framebuf_fd, -1);
    this->framebuf_varinfo = other.framebuf_varinfo;
    this->framebuf_fixinfo = other.framebuf_fixinfo;
    this->framebuf_ptr = std::exchange(other.framebuf_ptr, nullptr);
    this->next_update_marker = other.next_update_marker;
    return *this;
}

void screen_mxcfb::update(
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

void screen_mxcfb::update(mxcfb::waveform_modes mode, bool wait)
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

void screen_mxcfb::send_update(mxcfb::update_data& update, bool wait)
{
    update.update_marker = this->next_update_marker;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): Use of C library
    if (ioctl(this->framebuf_fd, mxcfb::send_update, &update) == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::screen_mxcfb::send_update) Screen update"
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
            "(rmioc::screen_mxcfb::send_update) Wait for update completion"
        );
    }

    if (this->next_update_marker == rmioc::screen_mxcfb::max_update_marker)
    {
        this->next_update_marker = 1;
    }
    else
    {
        ++this->next_update_marker;
    }
}

auto screen_mxcfb::get_data() -> std::uint8_t*
{
    return this->framebuf_ptr;
}

auto screen_mxcfb::get_xres() const -> int
{
    return this->framebuf_varinfo.xres;
}

auto screen_mxcfb::get_xres_memory() const -> int
{
    return this->framebuf_varinfo.xres_virtual;
}

auto screen_mxcfb::get_yres() const -> int
{
    return this->framebuf_varinfo.yres;
}

auto screen_mxcfb::get_yres_memory() const -> int
{
    return this->framebuf_varinfo.yres_virtual;
}

auto screen_mxcfb::get_bits_per_pixel() const -> unsigned short
{
    return this->framebuf_varinfo.bits_per_pixel;
}

auto screen_mxcfb::get_red_format() const -> component_format
{
    return component_format{
        static_cast<unsigned short>(this->framebuf_varinfo.red.offset),
        static_cast<unsigned short>(this->framebuf_varinfo.red.length),
    };
}

auto screen_mxcfb::get_green_format() const -> component_format
{
    return component_format{
        static_cast<unsigned short>(this->framebuf_varinfo.green.offset),
        static_cast<unsigned short>(this->framebuf_varinfo.green.length),
    };
}

auto screen_mxcfb::get_blue_format() const -> component_format
{
    return component_format{
        static_cast<unsigned short>(this->framebuf_varinfo.blue.offset),
        static_cast<unsigned short>(this->framebuf_varinfo.blue.length),
    };
}

} // namespace rmioc

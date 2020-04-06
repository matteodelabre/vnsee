#include "mxcfb.hpp"
#include "screen.hpp"
#include "log.hpp"
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <system_error>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace rm
{

screen::screen()
{
    this->framebuf_fd = open("/dev/fb0", O_RDWR);

    if (this->framebuf_fd == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rm::screen) Open device framebuffer"
        );
    }

    if (ioctl(
            this->framebuf_fd,
            FBIOGET_VSCREENINFO,
            &this->framebuf_varinfo
        ) == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rm::screen) Fetch framebuffer vscreeninfo"
        );
    }

    if (ioctl(
            this->framebuf_fd,
            FBIOGET_FSCREENINFO,
            &this->framebuf_fixinfo
        ) == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rm::screen) Fetch framebuffer fscreeninfo"
        );
    }

    this->framebuf_ptr = reinterpret_cast<uint8_t*>(mmap(
        /* addr = */ NULL,
        /* len = */ this->framebuf_fixinfo.smem_len,
        /* prot = */ PROT_READ | PROT_WRITE,
        /* flags = */ MAP_SHARED,
        /* fd = */ this->framebuf_fd,
        /* off = */ 0
    ));

    this->next_update_marker = 1;
}

screen::~screen()
{
    munmap(this->framebuf_ptr, this->framebuf_fixinfo.smem_len);
    this->framebuf_ptr = NULL;

    close(this->framebuf_fd);
    this->framebuf_fd = -1;
}

void screen::update(int x, int y, int w, int h)
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

    if (static_cast<std::uint32_t>(x + w) > this->get_xres())
    {
        w = this->framebuf_varinfo.xres - x;
    }

    if (static_cast<std::uint32_t>(y + h) > this->get_yres())
    {
        h = this->framebuf_varinfo.yres - y;
    }

    // Ignore out of bounds or null updates
    if (static_cast<std::uint32_t>(x) > this->get_xres()
        || static_cast<std::uint32_t>(y) > this->get_yres()
        || w == 0 || h == 0)
    {
        return;
    }

    log::print("Screen Update") << w << 'x' << h << '+' << x << '+' << y
        << '\n';

    mxcfb::update_data update;
    update.update_region.left = x;
    update.update_region.top = y;
    update.update_region.width = w;
    update.update_region.height = h;
    update.waveform_mode = mxcfb::waveform_modes::gc16;
    update.temp = 0x18;
    update.update_mode = mxcfb::update_modes::partial;
    update.flags = 0;
    update.update_marker = this->next_update_marker;

    if (ioctl(this->framebuf_fd, mxcfb::send_update, &update) == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rm::screen::update) Send update"
        );
    }

    mxcfb::update_marker_data wait;
    wait.update_marker = this->next_update_marker;
    wait.collision_test = 0;

    if (ioctl(this->framebuf_fd, mxcfb::wait_for_update_complete, &wait) == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rm::screen::update) Wait for update completion"
        );
    }

    if (this->next_update_marker == 255)
    {
        this->next_update_marker = 1;
    }
    else
    {
        ++this->next_update_marker;
    }
}

std::uint8_t* screen::get_data()
{
    return this->framebuf_ptr;
}

std::uint32_t screen::get_xres() const
{
    return this->framebuf_varinfo.xres;
}

std::uint32_t screen::get_xres_memory() const
{
    return this->framebuf_varinfo.xres_virtual;
}

std::uint32_t screen::get_yres() const
{
    return this->framebuf_varinfo.yres;
}

std::uint32_t screen::get_yres_memory() const
{
    return this->framebuf_varinfo.yres_virtual;
}

std::uint32_t screen::get_bits_per_pixel() const
{
    return this->framebuf_varinfo.bits_per_pixel;
}

std::uint32_t screen::get_red_offset() const
{
    return this->framebuf_varinfo.red.offset;
}

std::uint32_t screen::get_red_length() const
{
    return this->framebuf_varinfo.red.length;
}

std::uint32_t screen::get_red_max() const
{
    return (1 << this->framebuf_varinfo.red.length) - 1;
}

std::uint32_t screen::get_green_offset() const
{
    return this->framebuf_varinfo.green.offset;
}

std::uint32_t screen::get_green_length() const
{
    return this->framebuf_varinfo.green.length;
}

std::uint32_t screen::get_green_max() const
{
    return (1 << this->framebuf_varinfo.green.length) - 1;
}

std::uint32_t screen::get_blue_offset() const
{
    return this->framebuf_varinfo.blue.offset;
}

std::uint32_t screen::get_blue_length() const
{
    return this->framebuf_varinfo.blue.length;
}

std::uint32_t screen::get_blue_max() const
{
    return (1 << this->framebuf_varinfo.blue.length) - 1;
}

} // namespace rm

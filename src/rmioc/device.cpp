#include "device.hpp"
#include "screen_mxcfb.hpp"
#include "screen_rm2fb.hpp"
#include <fcntl.h>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace fs = std::filesystem;

namespace rmioc
{

device::device(
    types type,
    std::unique_ptr<buttons>&& buttons_device,
    std::unique_ptr<touch>&& touch_device,
    std::unique_ptr<pen>&& pen_device,
    std::unique_ptr<screen>&& screen_device
)
: type(type)
, buttons_device(std::move(buttons_device))
, touch_device(std::move(touch_device))
, pen_device(std::move(pen_device))
, screen_device(std::move(screen_device))
{}

auto get_device_type() -> device::types
{
    std::ifstream device_id_file{"/sys/devices/soc0/machine"};
    std::string device_id;
    std::getline(device_id_file, device_id);

    if (device_id == "reMarkable 1.0" || device_id == "reMarkable Prototype 1")
    {
        return device::types::reMarkable1;
    }

    if (device_id == "reMarkable 2.0")
    {
        return device::types::reMarkable2;
    }

    throw std::runtime_error{"Unknown reMarkable model (device identifier \
is “" + device_id + "”"};
}

void discover_input_devices(
    const char* base_path,
    device::types type,
    device_request request,
    std::unique_ptr<buttons>& buttons_device,
    std::unique_ptr<touch>& touch_device,
    std::unique_ptr<pen>& pen_device
)
{
    for (const auto& entry : fs::directory_iterator(base_path))
    {
        if (entry.is_character_file())
        {
            const char* path = entry.path().c_str();

            if (buttons::is(path))
            {
                if (request.has_buttons())
                {
                    buttons_device = std::make_unique<buttons>(path);
                }
            }
            else if (touch::is(path))
            {
                if (request.has_touch())
                {
                    touch_device = std::make_unique<touch>(
                        path,
                        /* flip_x = */ type == device::types::reMarkable1,
                        /* flip_y = */ true
                    );
                }
            }
            else if (pen::is(path))
            {
                if (request.has_pen())
                {
                    pen_device = std::make_unique<pen>(
                        path,
                        /* flip_x = */ false,
                        /* flip_y = */ true
                    );
                }
            }
        }
    }
}

auto device::detect(device_request request) -> device
{
    std::ifstream device_id_file{"/sys/devices/soc0/machine"};
    std::string device_id;
    std::getline(device_id_file, device_id);

    types type = get_device_type();
    std::unique_ptr<buttons> buttons_device;
    std::unique_ptr<touch> touch_device;
    std::unique_ptr<pen> pen_device;
    std::unique_ptr<screen> screen_device;

    // Use the appropriate screen driver based on current device type
    if (request.has_screen())
    {
        if (type == types::reMarkable1)
        {
            screen_device = std::make_unique<screen_mxcfb>("/dev/fb0");
        }
        else
        {
            constexpr auto rm2fb_msgqueue_key = 0x2257c;
            screen_device = std::make_unique<screen_rm2fb>(
                /* framebuf_path = */ "/swtfb.01",
                /* msgqueue_key = */ rm2fb_msgqueue_key
            );
        }
    }

    // Auto-detect and open requested input devices
    discover_input_devices(
        "/dev/input", type, request,
        buttons_device, touch_device, pen_device
    );

    return device(
        type,
        std::move(buttons_device),
        std::move(touch_device),
        std::move(pen_device),
        std::move(screen_device)
    );
}

auto device::get_type() const -> types
{
    return this->type;
}

auto device::get_buttons() -> buttons*
{
    return this->buttons_device.get();
}

auto device::get_touch() -> touch*
{
    return this->touch_device.get();
}

auto device::get_pen() -> pen*
{
    return this->pen_device.get();
}

auto device::get_screen() -> screen*
{
    return this->screen_device.get();
}

} // namespace rmioc

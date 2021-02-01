#include "device.hpp"
#include "screen_mxcfb.hpp"
#include <fstream>
#include <string>

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

auto device::detect(device_request request) -> device
{
    std::ifstream device_id_file{"/sys/devices/soc0/machine"};
    std::string device_id;
    std::getline(device_id_file, device_id);

    types type = types::reMarkable1;
    std::unique_ptr<buttons> buttons_device;
    std::unique_ptr<touch> touch_device;
    std::unique_ptr<pen> pen_device;
    std::unique_ptr<screen> screen_device;

    if (device_id == "reMarkable 1.0" || device_id == "reMarkable Prototype 1")
    {
        type = types::reMarkable1;

        if (request.has_buttons())
        {
            buttons_device = std::make_unique<buttons>("/dev/input/event2");
        }

        if (request.has_touch())
        {
            touch_device = std::make_unique<touch>("/dev/input/event1");
        }

        if (request.has_pen())
        {
            pen_device = std::make_unique<pen>("/dev/input/event0");
        }

        if (request.has_screen())
        {
            screen_device = std::make_unique<screen_mxcfb>("/dev/fb0");
        }
    }
    else if (device_id == "reMarkable 2.0")
    {
        type = types::reMarkable2;

        if (request.has_buttons())
        {
            buttons_device = std::make_unique<buttons>("/dev/input/event0");
        }

        if (request.has_touch())
        {
            touch_device = std::make_unique<touch>("/dev/input/event2");
        }

        if (request.has_pen())
        {
            pen_device = std::make_unique<pen>("/dev/input/event1");
        }

        /* if (request.has_screen()) */
        /* { */
            /* screen_device = std::make_unique<screen_rm2fb>("/swtfb.01"); */
        /* } */
    }
    else
    {
        throw std::runtime_error{"Unknown reMarkable model (device identifier \
says “" + device_id + "”"};
    }

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

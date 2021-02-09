#include "mxcfb.hpp"

namespace mxcfb
{

auto rect::clip(
    int left, int top,
    int width, int height,
    int xres, int yres
) -> rect
{
    if (left < 0)
    {
        width += left;
        left = 0;
    }

    if (top < 0)
    {
        height += top;
        top = 0;
    }

    if (left >= xres || top >= yres || width <= 0 || height <= 0)
    {
        return rect{0, 0, 0, 0};
    }

    if (left + width > xres)
    {
        width = xres - left;
    }

    if (top + height > yres)
    {
        height = yres - top;
    }

    return rect{
        static_cast<std::uint32_t>(top),
        static_cast<std::uint32_t>(left),
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),
    };
}

rect::operator bool() const
{
    return this->width > 0 && this->height > 0;
}

} // namespace mxcfb

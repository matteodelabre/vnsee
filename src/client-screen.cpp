#include "client.hpp"
#include "log.hpp"
#include "rmioc/screen.hpp"
#include <algorithm>
#include <chrono>
#include <ostream>
#include <rfb/rfbclient.h>
// IWYU pragma: no_include <type_traits>
// IWYU pragma: no_include "rfb/rfbproto.h"

namespace chrono = std::chrono;

/**
 * Time to wait after the last update from VNC before updating the screen
 * (in microseconds).
 *
 * VNC servers tend to send a lot of small updates in a short period of time.
 * This delay allows grouping those small updates into a larger screen update.
 */
constexpr chrono::milliseconds update_delay{150};

auto client::event_loop_screen() -> client::event_loop_status
{
    if (this->update_info.has_update != 0)
    {
        int remaining_wait_time =
            chrono::duration_cast<chrono::milliseconds>(
                this->update_info.last_update_time + update_delay
                - chrono::steady_clock::now()
            ).count();

        if (remaining_wait_time <= 0)
        {
            this->update_info.has_update = 0;

            log::print("Screen update")
                << this->update_info.w << 'x' << this->update_info.h << '+'
                << this->update_info.x << '+' << this->update_info.y << '\n';

            this->rm_screen.update(
                this->update_info.x, this->update_info.y,
                this->update_info.w, this->update_info.h
            );
        }
        else
        {
            // Wait until the next update is due
            return {false, remaining_wait_time};
        }
    }

    return {false, -1};
}

auto client::create_framebuf(rfbClient* /*unused*/) -> rfbBool
{
    // No-op: Data is written directly to the memory-mapped framebuffer
    return 1;
}

void client::update_framebuf(rfbClient* client, int x, int y, int w, int h)
{
    // Register the region as pending update, potentially extending
    // an existing one
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* update_info = reinterpret_cast<client::update_info_struct*>(
        rfbClientGetClientData(
            client,
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<void*>(client::update_info_tag)
        ));

    log::print("VNC update") << w << 'x' << h << '+' << x << '+' << y << '\n';

    if (update_info->has_update != 0)
    {
        // Merge new rectangle with existing one
        int left_x = std::min(x, update_info->x);
        int top_y = std::min(y, update_info->y);
        int right_x = std::max(x + w, update_info->x + update_info->w);
        int bottom_y = std::max(y + h, update_info->y + update_info->h);

        update_info->x = left_x;
        update_info->y = top_y;
        update_info->w = right_x - left_x;
        update_info->h = bottom_y - top_y;
    }
    else
    {
        update_info->x = x;
        update_info->y = y;
        update_info->w = w;
        update_info->h = h;
        update_info->has_update = 1;
    }

    update_info->last_update_time = chrono::steady_clock::now();
}

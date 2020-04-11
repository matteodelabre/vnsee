#ifndef RMIOC_TOUCH_HPP
#define RMIOC_TOUCH_HPP

#include "input.hpp"
#include <map>

namespace rmioc
{

class touch : public input
{
public:
    touch();

    /**
     * Check for new events.
     *
     * @return True if touch state changed since last call.
     */
    bool process_events();

    /** Current state of a touch point slot. */
    struct slot_state
    {
        int x;

        static constexpr int x_min = 0;
        static constexpr int x_max = 767;

        int y;

        static constexpr int y_min = 0;
        static constexpr int y_max = 1023;

        int pressure;

        static constexpr int pressure_min = 0;
        static constexpr int pressure_max = 255;

        int orientation;

        static constexpr int orientation_min = -127;
        static constexpr int orientation_max = 127;
    };

    using slots_state_t = std::map<int, slot_state>;

    const slots_state_t& get_slots_state() const;

private:
    /** List of active touch point slots indexed by their ID. */
    slots_state_t slots_state;

    /** Currently active touch point slot. */
    int current_slot = 0;
}; // class touch

} // namespace rmioc

#endif // RMIOC_TOUCH_HPP

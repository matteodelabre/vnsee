#ifndef INPUT_HPP
#define INPUT_HPP

#include <map>
#include <vector>

struct input_event;

namespace rm
{

class input
{
public:
    input();
    ~input();

    /**
     * Check for new events.
     *
     * @return True if touch state changed since last call.
     */
    bool fetch_events();

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
    const slots_state_t& get_previous_slots_state() const;

private:
    /** File descriptor for the input device. */
    int input_fd;

    /** List of events to be processed. */
    std::vector<input_event> pending_events;

    /** List of active touch point slots indexed by their ID. */
    slots_state_t slots_state;

    /** Previous state of active touch point slots. */
    slots_state_t previous_slots_state;

    /** Currently active touch point slot. */
    int current_slot = 0;
};

void read_input();

}

#endif // INPUT_HPP

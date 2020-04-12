#ifndef RMIOC_BUTTONS_HPP
#define RMIOC_BUTTONS_HPP

#include "input.hpp"

namespace rmioc
{

/**
 * Access to the state of the device’s physical buttons.
 */
class buttons : public input
{
public:
    buttons();

    /**
     * Fetch new events from the buttons and process them.
     *
     * @return True if the buttons’ state changed since last call.
     */
    bool process_events();

    /**
     * Information about each physical button of the device.
     *
     * There are four buttons in total, including the power button at the top
     * and three buttons, left, home and right, below the screen.
     *
     * Each button’s state is indicated by a boolean set to true when the
     * button is pressed and false otherwise.
     */
    struct buttons_state
    {
        bool left = false;
        bool home = false;
        bool right = false;
        bool power = false;
    };

    const buttons_state& get_state() const;

private:
    buttons_state state;
}; // class buttons

} // namespace rmioc

#endif // RMIOC_BUTTONS_HPP

#pragma once

#include <inttypes.h>

/* Store one bit of information for each button.
 * This can be used to e.g. track which buttons are currently being held down.
 *
 * Each bit is either zero (cleared) or one (set).
 * Buttons are referred to by their index (0-7).
 * The bits are stored in a u8 with button 0 in the LSB and button 7 in the MSB.
 */
class ButtonsBits
{
public:
    ButtonsBits();

    /************************
     * Clearing
     ************************/

    /* Clear all bits. */
    void clear_all();

    /* Clear the bit for a specific button. */
    void clear(int button_idx);

    /* Clear the bits for multiple buttons by specifying a mask.
     * For each bit in the mask that is set, the corresponding button will be cleared. */
    void clear_raw(uint8_t mask);

    /************************
     * Setting
     ************************/

    /* Set the bit for a specific button. */
    void set(int button_idx);

    /* Set the bits for multiple buttons by specifying a mask.
     * For each bit in the mask that is set, the corresponding button will be set. */
    void set_raw(uint8_t mask);

    /************************
     * Getting and Testing
     ************************/

    /* Get the bits as stored in a u8. */
    uint8_t get_raw();

    /* Is the bit for this specific button set?
     *
     * @param[in] button_idx The button to test.
     * @return true if set.
     */
    bool is_set(int button_idx);

    /* Is this specific button the only one with a bit set?
     *
     * @param[in] button_idx The button to test.
     * @return true if its bit is set, and no other buttons have a bit set.
     */
    bool is_single_set(int button_idx);

    /* Is there any button which is the only one with a bit set?
     *
     * @param[out] button_idx The matching button, if there is one.
     * @return true if there is one button with its bit, and no other buttons have a bit set.
     */
    bool get_single_set(int &button_idx);

    /* Are any bits set? */
    bool is_any_set();

    /* Are all of the bits clear? */
    bool is_all_clear();

private:
    uint8_t button_bits;
};

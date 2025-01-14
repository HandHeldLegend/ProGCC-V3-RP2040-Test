#include "progcc_includes.h"

// Set up local input vars
progcc_button_data_s button_data = {0};
a_data_s analog_data = {0};
a_data_s scaled_analog_data = {0};

bool calibrate = true;
bool centered = false;

void main_two()
{
    for(;;)
    {
        progcc_utils_read_buttons(&button_data);
        progcc_utils_read_sticks(&analog_data);

        if(calibrate)
        {
            if (!centered)
            {
                stick_scaling_capture_center(&analog_data);
                centered = true;
                stick_scaling_print_centers();
            }

            stick_scaling_create_scalers(&analog_data);

            if (!gpio_get(PGPIO_BUTTON_A))
            {
                calibrate = false;
                progcc_utils_set_rumble(PROGCC_RUMBLE_ON);
                sleep_ms(200);
                progcc_utils_set_rumble(PROGCC_RUMBLE_OFF);
            }
        }
        else
        {
            stick_scaling_process_data(&analog_data, &scaled_analog_data);
        }
    }
}

int main() {
    stdio_init_all();
    board_init();

    sleep_ms(200);

    // Perform GPIO setup
    progcc_utils_hardware_setup();

    if (!gpio_get(PGPIO_BUTTON_START))
    {
        reset_usb_boot(0, 0);
    }

    progcc_usb_set_mode(PUSB_MODE_XI, true);

    if (!progcc_usb_start())
    {
        // Fall back to bootloader if we fail to start.
        reset_usb_boot(0, 0);
    }

    sleep_ms(200);

    multicore_launch_core1(main_two);

    //printf("Testing");
    for(;;)
    {
        progcc_usb_task(&button_data, &scaled_analog_data);
        tud_task();
        //printf("Test");
        sleep_ms(1);
    }

}

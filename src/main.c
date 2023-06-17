#include "progcc_includes.h"

// redefine _write() so that printf() outputs to UART
int _write(int file, char *ptr, int len) {
    int i;
    for (i = 0; i < len; i++) {
        uart_putc_raw(uart0, ptr[i]);
    }
    return len;
}

// Set up local input vars
progcc_button_data_s button_data = {0};
a_data_s analog_data = {0};
a_data_s scaled_analog_data = {0};

bool calibrate = false;
bool centered = false;

void write_color(uint32_t col)
{
    uint32_t n = col | 0xFF000000;
    for (uint8_t i = 0; i < NUM_PIXELS; i++)
    {
        put_pixel(n);
    }
}

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
            }

            stick_scaling_capture_distances(&analog_data);

            /*
            if (!gpio_get(PGPIO_BUTTON_MODE))
            {
                stick_scaling_finalize();
                calibrate = false;
                progcc_utils_set_rumble(PROGCC_RUMBLE_ON);
                sleep_ms(200);
                progcc_utils_set_rumble(PROGCC_RUMBLE_OFF);
            }*/
        }
        else
        {
            stick_scaling_process_data(&analog_data, &scaled_analog_data);
        }
    }
}

int main() {
    stdio_init_all();
    sleep_ms(100);
    board_init();

    printf("ProGCC Started.\n");

    switch_analog_calibration_init();

    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 1100000, IS_RGBW);

    stick_scaling_init();

    sleep_ms(200);

    // Perform GPIO setup
    progcc_utils_hardware_setup();

    if(!gpio_get(PGPIO_BUTTON_MODE))
    {
        reset_usb_boot(0, 0);
    }

    // Flash LEDS and do rumble
    progcc_utils_set_rumble(PROGCC_RUMBLE_ON);
    sleep_ms(200);
    progcc_utils_set_rumble(PROGCC_RUMBLE_OFF);
    sleep_ms(200);
    progcc_utils_set_rumble(PROGCC_RUMBLE_ON);
    sleep_ms(200);
    progcc_utils_set_rumble(PROGCC_RUMBLE_OFF);
    sleep_ms(200);
    write_color(0xFF0000);
    sleep_ms(500);
    write_color(0xFF00);
    sleep_ms(500);
    write_color(0xFF);
    sleep_ms(500);
    write_color(0x00);
    sleep_ms(500);

    write_color(0xFFFFFF);
    sleep_ms(100);
    write_color(0x00);
    sleep_ms(100);
    write_color(0xFFFFFF);
    sleep_ms(100);
    write_color(0x00);

    progcc_usb_set_mode(PUSB_MODE_XI, true);

    if (!progcc_usb_start())
    {
        // Fall back to bootloader if we fail to start.
        //reset_usb_boot(0, 0);
    }

    sleep_ms(200);

    // Stick init
    progcc_utils_read_sticks(&analog_data);
    stick_scaling_capture_center(&analog_data);
    stick_scaling_finalize();

    multicore_launch_core1(main_two);

    //printf("Testing");
    for(;;)
    {
        progcc_usb_task(&button_data, &scaled_analog_data);
        tud_task();
        //printf("Test");
        sleep_ms(8);
    }

}

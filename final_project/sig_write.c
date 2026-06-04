#include "rpi.h"
#include "gpio.h"
#include "i2c.h"
#include "ssd1306-display-driver.h"
#include "sig-save.h"

// TSC2007 I2C address and command bytes.
// cmd = (function << 4) | (power_mode << 2) | (resolution << 1)
// Source: https://github.com/adafruit/Adafruit_TSC2007/blob/master/Adafruit_TSC2007.cpp
//
// function values:  MEASURE_Z1=14, MEASURE_Z2=15, MEASURE_X=12, MEASURE_Y=13
// power modes:      POWERDOWN_IRQON=0, ADON_IRQOFF=1
// resolution:       ADC_12BIT=0
//
// During measurements use ADON_IRQOFF (keeps ADC running between reads).
// After all reads, power down with cmd=0x00.

#define TSC2007_ADDR   0x48

#define CMD_Z1         0xE4   // (14<<4)|(1<<2) — Z1 pressure, ADC on
#define CMD_Z2         0xF4   // (15<<4)|(1<<2) — Z2 pressure, ADC on
#define CMD_X          0xC4   // (12<<4)|(1<<2) — X position, ADC on
#define CMD_Y          0xD4   // (13<<4)|(1<<2) — Y position, ADC on
#define CMD_POWERDOWN  0x00   // ( 0<<4)|(0<<2) — power down, IRQ on // we are not constantly polling when waiting for new sig. we wait for interrupt to tell us 

// GPIO17 (pin 11) wired to button, other side to GND
#define BUTTON_PIN     17

// 15 seconds of no touch triggers auto-save
#define TIMEOUT_USEC   15000000

// Low-level: send one command, wait 500us, read back 12-bit result.
static uint16_t tsc2007_cmd(uint8_t cmd) {
    i2c_write(TSC2007_ADDR, &cmd, 1); // telling tsc2007 to measure x 
    delay_us(500);                                       // ADC conversion time
    uint8_t buf[2];
    i2c_read(TSC2007_ADDR, buf, 2); // this gives us value from 0-4095 (12bit value)
    return ((uint16_t)buf[0] << 4) | (buf[1] >> 4);    // extract 12-bit result
}

// Read a touch sample. Returns 1 if finger is down, 0 if not.
// Mirrors Adafruit read_touch(): reads X and Y twice, rejects if
// consecutive readings differ by >100 (filters noise/flicker on pen-up).
static int tsc2007_read_touch(uint16_t *x, uint16_t *y,
                               uint16_t *z1, uint16_t *z2) {
    *z1 = tsc2007_cmd(CMD_Z1); // voltage measured from one side of screen
    *z2 = tsc2007_cmd(CMD_Z2); // voltage measured from the other side

    uint16_t x1 = tsc2007_cmd(CMD_X); // we are measuring two values because we want to determine if this is noise.
    uint16_t y1 = tsc2007_cmd(CMD_Y);
    uint16_t x2 = tsc2007_cmd(CMD_X);
    uint16_t y2 = tsc2007_cmd(CMD_Y);

    tsc2007_cmd(CMD_POWERDOWN);   // power down, re-enable IRQ

    // reject unstable reading (finger lifting causes flicker)
    int32_t dx = (int32_t)x1 - (int32_t)x2;
    int32_t dy = (int32_t)y1 - (int32_t)y2;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    if (dx > 100 || dy > 100) return 0;

    *x = x1;
    *y = y1;

    // x==4095 or y==4095 means lines floating = not touched
    return (*x != 4095) && (*y != 4095);
}

// Returns 1 if button pressed (with 50ms debounce).
static int button_is_pressed(void) {
    if (gpio_read(BUTTON_PIN) != 0) return 0;
    delay_ms(50);
    return gpio_read(BUTTON_PIN) == 0;
}

// Show a short message centered on OLED, then pause.
static void oled_message(const char *msg, uint32_t ms) {
    ssd1306_display_clear();
    uint16_t x = 10;
    for (int i = 0; msg[i]; i++, x += 8)
        ssd1306_display_draw_character_size(x, 28, msg[i], COLOR_WHITE, 1, 1);
    ssd1306_display_show();
    delay_ms(ms);
}

void notmain(void) {
    // 1. initialize SD card, FAT32, file counter, pixel buffer
    sig_init();

    // 2. initialize I2C (shared by OLED at 0x3C and TSC2007 at 0x48)
    i2c_init();

    // 3. initialize OLED
    ssd1306_display_init();
    ssd1306_display_clear();
    ssd1306_display_show();

    // 4. initialize done button
    gpio_set_input(BUTTON_PIN);
    gpio_set_pullup(BUTTON_PIN);

    int      has_strokes     = 0;
    int      point_count     = 0;
    uint32_t last_touch_usec = 0;

    oled_message("Ready!", 1000);
    ssd1306_display_clear();
    ssd1306_display_show();
    printk("sig_write: waiting for signatures\n");

    while (1) {
        uint16_t x, y, z1, z2;
        int touched = tsc2007_read_touch(&x, &y, &z1, &z2); // get one coordinat

        if (touched) {
            // draw on OLED: scale touch (0-4095) to OLED pixels (128x64)
            uint16_t ox = x * SSD1306_DISPLAY_WIDTH  / 4096;
            uint16_t oy = y * SSD1306_DISPLAY_HEIGHT / 4096;
            ssd1306_display_draw_pixel(ox, oy, COLOR_WHITE);
            point_count++;
            if (point_count % 5 == 0)               // OLED: update every 5 points
                ssd1306_display_show();

            // record into BMP pixel buffer (1024x512)
            sig_draw_point(x, y); // this function does the scaling.  

            last_touch_usec = timer_get_usec();
            has_strokes = 1;
        }

        // nothing drawn yet, keep waiting
        if (!has_strokes) continue;

        uint32_t elapsed = timer_get_usec() - last_touch_usec;
        int timed_out    = (elapsed > TIMEOUT_USEC);

        if (button_is_pressed() || timed_out) {
            printk("sig_write: saving...\n");

            sig_save();   // write BMP to SD card (SIG000.BMP, SIG001.BMP, ...)
            sig_clear();  // clear pixel buffer white for next person

            oled_message("Saved!", 2000);
            ssd1306_display_clear();
            ssd1306_display_show();
            has_strokes = 0;
            point_count = 0;

            printk("sig_write: ready for next signature\n");
        }
    }
}

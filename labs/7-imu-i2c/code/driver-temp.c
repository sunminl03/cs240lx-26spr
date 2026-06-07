// engler: simplistic mpu6050 gyro driver code, mirrors
// <driver-accel.c>.  
//  1. initializes gyroscope,
//  2. prints N readings.
//
// as with <driver-accel.c>:
//   - <mpu-6050.h> has the interface description.
//   - <mpu-6050.c> is where your code will go.
//
//
// Obvious extension:
//  0. validate the 'data ready' matches what we set it to.
//  1. device interrupts.
//  2. bit bang i2c
//  3. multiple devices.
//  4. extend the interface to give more control.
//  
//
// KEY: document why you are doing what you are doing.
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
//  **** put page numbers for any device-specific things you do ***
// 
// also: a sentence or two will go a long way in a year when you want 
// to re-use the code.
#include "rpi.h"
#include "mpu-6050.h"
#include "mbox.h"

void notmain(void) {
    delay_ms(100);   // allow time for i2c/device to boot up.
    i2c_init();
    delay_ms(100);   // allow time for i2c/dev to settle after init.

    // from application note.
    uint8_t dev_addr = 0b1101000;

    enum { 
        WHO_AM_I_REG      = 0x75, 
        WHO_AM_I_VAL = 0x68,       
    };

    uint8_t v = imu_rd(dev_addr, WHO_AM_I_REG);
    if(v != WHO_AM_I_VAL)
        panic("Initial probe failed: expected %b (%x), have %b (%x)\n", 
            WHO_AM_I_VAL, WHO_AM_I_VAL, v, v);
    printk("SUCCESS: mpu-6050 acknowledged our ping: WHO_AM_I=%b!!\n", v);

    // hard reset: it won't be when your pi reboots.
    mpu6050_reset(dev_addr);

    // extension: compare temperature with pi's temp
    int temp_c = temp_measure(dev_addr);
    printk("Temperature of MPU6050: %d C\n", temp_c);

    unsigned x = rpi_temp_get();
    // convert <x> to C and F
    unsigned C = 0, F = 0;
    C = x / 1000;
    printk("Temperature of RPI: %d C\n", C);

}

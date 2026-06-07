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
#include "stdlib.h" // Required for abs()
#include <math.h> 

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

    // part 3: for self test, dps = 250 
    gyro_t g = mpu6050_gyro_init(dev_addr, gyro_250dps);
    assert(g.dps==250);
    imu_xyz_t original[20];
    for(int i = 0; i < 20; i++) {
        imu_xyz_t xyz_raw = gyro_rd(&g);
        original[i] = xyz_raw;
        
        output("reading gyro %d\n", i);
        xyz_print("\traw", xyz_raw);
        xyz_print("\tscaled (milli dps)", gyro_scale(&g, xyz_raw));
        delay_ms(1000);
    }
    output("done reading without selfcheck\n");

    // SELF CHECK 
    mpu6050_gyro_selftest_init(dev_addr); // set self test config
    delay_ms(250);
    assert(g.dps==250);
    imu_xyz_t selftest[20];
    output("starting reading selftest vals\n");
    for(int i = 0; i < 40; i++) {
        imu_xyz_t xyz_raw = gyro_rd(&g);
        if (i >= 20) {
            selftest[i-20] = xyz_raw;
        } 
        output("reading gyro %d\n", i);
        xyz_print("\traw", xyz_raw);
        xyz_print("\tscaled (milli dps)", gyro_scale(&g, xyz_raw));
        delay_ms(1000);
    }
    
    imu_xyz_t self_reg_vals = gyro_rd_selftest(&g);
    float ft_z =  25. * 131. * powf(1.046, self_reg_vals.z - 1.);
    float ft_x =  25. * 131. * powf(1.046, self_reg_vals.x - 1.);
    float ft_y = -25. * 131. * powf(1.046, self_reg_vals.y - 1.);
    for (int i = 0; i < 20; i ++) {
        float r_z = (selftest[i].z - original[i].z - ft_z) / ft_z;
        float r_y = (selftest[i].y - original[i].y - ft_y) / ft_y;
        float r_x = (selftest[i].x - original[i].x - ft_x) / ft_x;

        if ((r_z) > 0.14 || (r_z) < -0.14 || r_y > 0.14 || r_y < -0.14|| r_x > 0.14 || r_x <-0.14) {
            output("Reading %d is NOT ACCEPTABLE\n", i);
        } else {
            output("Reading %d is ACCEPTABLE\n", i);
        }
    }

}

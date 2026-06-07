
#include "rpi.h"
#include "i2c.h"
#include "ssd1306-display-driver.h"

void notmain(void) {
  // Initialize I2C with some settling time
  delay_ms(100);
  i2c_init_clk_div(1500);
  delay_ms(100);

  // Initialize the display with some settling time
  ssd1306_display_init();
  delay_ms(100);



  // // Draw individual pixel
  ssd1306_display_draw_pixel(0, 0, COLOR_WHITE);

  // // Draw horizontal line
  // ssd1306_display_draw_horizontal_line(0, 100, 20, COLOR_WHITE);

  // moving vertical line
  // for (int i = 0; i < 50; i++) {

  // right ear
  ssd1306_display_draw_vertical_line(10, 25, 30, COLOR_WHITE); // right ear
  for (int i = 0; i < 10; i++) {
    ssd1306_display_draw_pixel(30 + i, 10 + i, COLOR_WHITE);
  }
  // left ear
  ssd1306_display_draw_vertical_line(10, 25, 128-30, COLOR_WHITE);
  for (int i = 0; i < 10; i++) {
    ssd1306_display_draw_pixel(128-30 - i, 10 + i, COLOR_WHITE);
  }
  // right ear head 
  int xcoor = 128-30-9; 
  for (int i = 0; i < 6; i++) {
    ssd1306_display_draw_pixel(xcoor - 1, 10 + 9 - i, COLOR_WHITE);
    ssd1306_display_draw_pixel(xcoor - 2, 10 + 9 - i, COLOR_WHITE);
    ssd1306_display_draw_pixel(xcoor - 3, 10 + 9 - i, COLOR_WHITE);
    xcoor = xcoor -3;
  }
  ssd1306_display_draw_pixel(xcoor - 1, 10 + 9 -5, COLOR_WHITE);
  ssd1306_display_draw_pixel(xcoor - 2, 10 + 9 - 5, COLOR_WHITE);
  ssd1306_display_draw_pixel(xcoor - 3, 10 + 9 - 5, COLOR_WHITE);
  ssd1306_display_draw_pixel(xcoor - 4, 10 + 9 - 5, COLOR_WHITE);
  ssd1306_display_draw_pixel(xcoor - 5, 10 + 9 - 5, COLOR_WHITE);

  // left ear head
  xcoor = 39;
  for (int i = 0; i < 6; i++) {
    ssd1306_display_draw_pixel(xcoor + 1, 10 + 9 - i, COLOR_WHITE);
    ssd1306_display_draw_pixel(xcoor + 2, 10 + 9 - i, COLOR_WHITE);
    ssd1306_display_draw_pixel(xcoor + 3, 10 + 9 - i, COLOR_WHITE);
    xcoor = xcoor +3;
  }
  ssd1306_display_draw_pixel(xcoor +1, 10 + 9 -5, COLOR_WHITE);
  ssd1306_display_draw_pixel(xcoor + 2, 10 + 9 - 5, COLOR_WHITE);
  ssd1306_display_draw_pixel(xcoor + 3, 10 + 9 - 5, COLOR_WHITE);
  ssd1306_display_draw_pixel(xcoor + 4, 10 + 9 - 5, COLOR_WHITE);
  ssd1306_display_draw_pixel(xcoor + 5, 10 + 9 - 5, COLOR_WHITE);


  // left chin y = -10x + 325
  ssd1306_display_draw_vertical_line(25, 30, 30, COLOR_WHITE);
  ssd1306_display_draw_vertical_line(30, 35, 29, COLOR_WHITE);
  ssd1306_display_draw_vertical_line(35, 40, 28, COLOR_WHITE);
  ssd1306_display_draw_vertical_line(40, 45, 27, COLOR_WHITE);
  ssd1306_display_draw_vertical_line(45, 50, 26, COLOR_WHITE);
  ssd1306_display_draw_vertical_line(50, 55, 25, COLOR_WHITE);

  xcoor = 25;
  int ycoor = 55;
  for (int i = 0; i < 12; i++) {
    ssd1306_display_draw_horizontal_line(xcoor, xcoor+3, ycoor + i, COLOR_WHITE);
    xcoor = xcoor + 3; 
  }
  // left whiskers
  ssd1306_display_draw_horizontal_line(17, 37, 40, COLOR_WHITE);
  ssd1306_display_draw_horizontal_line(15, 35, 45, COLOR_WHITE);
  ssd1306_display_draw_horizontal_line(13, 33, 50, COLOR_WHITE);

  // left eyebrow
  xcoor = 42;
  ycoor = 30;
  for (int i = 0; i < 5; i++) {
    ssd1306_display_draw_horizontal_line(xcoor, xcoor+3, ycoor + i, COLOR_WHITE);
    xcoor = xcoor + 3; 
  }


  // left eye
  ssd1306_display_draw_fill_rect(xcoor-6, ycoor+7, 5, 7, COLOR_WHITE);


  // right chin 
  ssd1306_display_draw_vertical_line(25, 30, 98, COLOR_WHITE);
  ssd1306_display_draw_vertical_line(30, 35, 99, COLOR_WHITE);
  ssd1306_display_draw_vertical_line(35, 40, 100, COLOR_WHITE);
  ssd1306_display_draw_vertical_line(40, 45, 101, COLOR_WHITE);
  ssd1306_display_draw_vertical_line(45, 50, 102, COLOR_WHITE);
  ssd1306_display_draw_vertical_line(50, 55, 103, COLOR_WHITE);

  xcoor = 103;
  ycoor = 55;
  for (int i = 0; i < 12; i++) {
    ssd1306_display_draw_horizontal_line(xcoor-3, xcoor, ycoor + i, COLOR_WHITE);
    xcoor = xcoor - 3; 
  }
  // right whiskers
  ssd1306_display_draw_horizontal_line(90, 110, 40, COLOR_WHITE);
  ssd1306_display_draw_horizontal_line(92, 112, 45, COLOR_WHITE);
  ssd1306_display_draw_horizontal_line(94, 114, 50, COLOR_WHITE);

  // right eyebrow
  xcoor = 87;
  ycoor = 30;
  for (int i = 0; i < 5; i++) {
    ssd1306_display_draw_horizontal_line(xcoor-3, xcoor, ycoor + i, COLOR_WHITE);
    xcoor = xcoor - 3; 
  }
  // right eye
  ssd1306_display_draw_fill_rect(xcoor+3, ycoor+7, 5, 7, COLOR_WHITE);

  // nose 
  xcoor = 62; 
  ycoor = 50;
  for (int i = 0; i < 6; i++) {
    ssd1306_display_draw_horizontal_line(xcoor + i, xcoor + 6 - i, ycoor + i, COLOR_WHITE);
  }

  // for (int i = 30; i > 25; i--) {
  //   int j = -10 * i + 325;
  //   ssd1306_display_draw_pixel(i, j, COLOR_WHITE);
  // }
  // ssd1306_display_draw
  // _horizontal_line(0, 100, i, COLOR_WHITE);
  ssd1306_display_show();
  // ssd1306_display_clear();
  delay_ms(100);
  // }


  

  // // Draw filled rectangle
  // ssd1306_display_draw_fill_rect(40, 30, 50, 20, COLOR_WHITE);

  // Draw some text
//   char *text = "hello cs340lx!";
//   for (int i = 0; i < strlen(text); i++) {
//     ssd1306_display_draw_character_size(5 + 5 * i, 10, text[i], COLOR_WHITE, 1, 1);
//   }

//   // Draw some bigger text
//   ssd1306_display_draw_character_size(10, 40, 'A', COLOR_WHITE, 2, 2);

  // Call display_show() to actually update the display
//   ssd1306_display_show();

  while (1) {}
}

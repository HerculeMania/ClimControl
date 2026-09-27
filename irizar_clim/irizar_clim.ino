#include <U8glib.h>
const uint8_t wr = 1, rd = 0, cs = 2, a0 = 3, reset = A4, d0 = A3, d1 = 4, d2 = 5, d3 = 6, d4 = 7, d5 = A2, d6 = A1, d7 = A0;
//U8GLIB_T6963_240X128 u8g(8, 9, 10, 11, 4, 5, 6, 7, 14, 15, 17, 18, 16); // 8Bit Com: D0..D7: 8,9,10,11,4,5,6,7, cs=14, a0=15, wr=17, rd=18, reset=16
//U8GLIB_T6963_128X128 u8g(8, 9, 10, 11, 4, 5, 6, 7, 14, 15, 17, 18, 16); // 8Bit Com: D0..D7: 8,9,10,11,4,5,6,7, cs=14, a0=15, wr=17, rd=18, reset=16
//U8GLIB_T6963_240X64 u8g(d0, d1, d2, d3, d4, d5, d6, d7, cs, a0, wr, rd, reset);  // 8Bit Com: D0..D7: 8,9,10,11,4,5,6,7, cs=2, a0=11, wr=1, rd=0, reset=A4
//U8GLIB_T6963_128X64 u8g(8, 9, 10, 11, 4, 5, 6, 7, 14, 15, 17, 18, 16); // 8Bit Com: D0..D7: 8,9,10,11,4,5,6,7, cs=14, a0=15, wr=17, rd=18, reset=16
U8GLIB_T6963_240X128 u8g(d0, d1, d2, d3, d4, d5, d6, d7, cs, a0, wr, rd, reset);
void draw(uint8_t y) {
  // graphic commands to redraw the complete screen should be placed here
  //u8g.setFont(u8g_font_unifont);
  //u8g.setFont(u8g_font_osb21);
  //u8g.drawStr(0, 22, "Hello World!");

  u8g.setFont(u8g_font_osb21);
  u8g.drawStr(1, 20, "Je suis mania");
}

void setup(void) {
  // flip screen, if required
  // u8g.setRot180();

  // set SPI backup if required
  //u8g.setHardwareBackup(u8g_backup_avr_spi);

  // assign default color value
  if (u8g.getMode() == U8G_MODE_R3G3B2) {
    u8g.setColorIndex(255);  // white
  } else if (u8g.getMode() == U8G_MODE_GRAY2BIT) {
    u8g.setColorIndex(3);  // max intensity
  } else if (u8g.getMode() == U8G_MODE_BW) {
    u8g.setColorIndex(1);  // pixel on
  } else if (u8g.getMode() == U8G_MODE_HICOLOR) {
    u8g.setHiColorByRGB(255, 255, 255);
  }

  pinMode(8, OUTPUT);
}
uint8_t ypos = 1;
void loop(void) {
  // picture loop
  u8g.firstPage();
  do {
    draw(ypos++ != 128 ? ypos : ypos = 22);
    delay(20);
  } while (u8g.nextPage());

  // rebuild the picture after some delay
  //delay(50);
}
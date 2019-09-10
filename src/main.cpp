#include "Arduino.h"
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R3, /* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

void setup()
{
  u8g2.begin();
}

void loop()
{
  u8g2.clearBuffer();					// clear the internal memory
  u8g2.setFont(u8g2_font_t0_13b_tf);	// choose a suitable font
  u8g2.drawStr(0,20,"Hello World!");	// write something to the internal memory
  u8g2.sendBuffer();					// transfer internal memory to the display
  delay(1000);
}

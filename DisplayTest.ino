#include <U8g2lib.h>
#include <Wire.h>
#define time_delay 2000
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void u8g2_prepare() {
u8g2.setFont(u8g2_font_6x10_tf);
u8g2.setFontRefHeightExtendedText();
u8g2.setDrawColor(1);
u8g2.setFontPosTop();
u8g2.setFontDirection(0);
}

void setup() {
 u8g2.begin();
 u8g2_prepare();
}

void loop() {
u8g2.clearBuffer();
u8g2.drawStr(0, 0, "Chip e chop UN CAZZO");
u8g2.sendBuffer();
delay(time_delay);
}

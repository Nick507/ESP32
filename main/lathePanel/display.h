#pragma once

#include <stdint.h>
#include "gfxfont.h"

#define DISPLAY_WIDTH   480
#define DISPLAY_HEIGHT  320

#define TFT_WHITE       0xFFFF
#define TFT_GRAY_224    0xE71C
#define TFT_GRAY_192    0xC618
#define TFT_GRAY_160    0xA514
#define TFT_GRAY_128    0x8410
#define TFT_BLACK       0x0000
#define TFT_BLUE        0x001F
#define TFT_RED         0xF800

#ifdef __cplusplus
extern "C" {
#endif

// hardware dependant functions
void hdDisplayInit();
void displayRect(int16_t x, int16_t y, uint16_t sx, uint16_t sy, uint16_t color);
void hdDisplayStartWindow(int16_t x, int16_t y, uint16_t sx, uint16_t sy);
void hdDisplayPushPixelToWindow(uint16_t color);

// common functions
void displayInit(void);
void displayPrintf(const char* format, ...);
void displayClear(uint16_t color);
void displaySetCursor(int16_t x, int16_t y);
void displayGetCursor(int16_t * x, int16_t * y);
bool displayReadTouch(int16_t * x, int16_t * y);
void displayLine(int16_t x1, int16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void displaySetTextColor(uint16_t fg, uint16_t bg);
void displaySetTextFont(const GFXfont * font);
void displayGetTextBounds(const char * format, uint16_t * sx, uint16_t * sy, int16_t * baseline, ...);
int displayGetFontHeight();
void displayPutPixel(int16_t x, int16_t y, uint16_t color);

#ifdef __cplusplus
}
#endif

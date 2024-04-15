#include "display.h"
#include <stdio.h>
#include <cstdarg>

static int16_t cursorX = 0, cursorY = 0;
static uint16_t fgTextColor, bgTextColor;
const GFXfont * gfxFont = NULL;

void displaySetCursor(int16_t x, int16_t y)
{
    cursorX = x;
    cursorY = y;
}

void displayGetCursor(int16_t * x, int16_t * y)
{
    *x = cursorX;
    *y = cursorY;
}

void displayInit(void)
{
    hdDisplayInit();
    displayClear(TFT_BLACK);
}

void displayClear(uint16_t color)
{
    displayRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, color);
    displaySetCursor(0, 0);
}

void displaySetTextColor(uint16_t fg, uint16_t bg)
{
    fgTextColor = fg;
    bgTextColor = bg;
}

void displaySetTextFont(const GFXfont * font)
{
    gfxFont = font;
}

void displayGetTextBounds(const char * format, uint16_t * sx, uint16_t * sy, int16_t * baseline, ...)
{
    if(!format)
    {
        *sx = 0;
        *sy = 0;
        return;
    }

    char buf[64];
    va_list varArgs;

    va_start(varArgs, baseline);
    vsnprintf(buf, sizeof(buf), format, varArgs);
    va_end(varArgs);

    *sx = 0;
    int8_t minY = 127, maxY = -127;
    for(int i = 0; buf[i]; ++i)
    {
        GFXglyph &glyph = gfxFont->glyph[buf[i] - gfxFont->first];
        *sx += glyph.xAdvance;
        if(glyph.yOffset < minY) minY = glyph.yOffset;
        if(glyph.yOffset + glyph.height > maxY) maxY = glyph.yOffset + glyph.height;
    }
    
    //*sy = gfxFont->yAdvance;
    *sy = maxY - minY;
    if(baseline) *baseline = maxY;
}

int displayGetFontHeight()
{
    return gfxFont->yAdvance;
}

void displayChar(uint8_t c)
{
    if(!gfxFont) return;

    if(c != '\n')
    {
        c -= gfxFont->first;
        GFXglyph &glyph = gfxFont->glyph[c];
        uint8_t *bitmap = gfxFont->bitmap + glyph.bitmapOffset;
        uint8_t w = glyph.width, h = glyph.height, b;

        if(cursorX + glyph.xAdvance > DISPLAY_WIDTH)
        {
            cursorX = 0;
            cursorY += gfxFont->yAdvance;
        }
        if(cursorY > DISPLAY_HEIGHT) cursorY = gfxFont->yAdvance;

        if(w > 0 && h > 0)
        {
            hdDisplayStartWindow(cursorX + glyph.xOffset, cursorY + glyph.yOffset, w, h);

            for(int i = 0; i < w * h; ++i)
            {
                if((i & 7) == 0) b = *bitmap++;
                hdDisplayPushPixelToWindow(b & 0x80 ? fgTextColor : bgTextColor);
                b = b << 1;
            }
        }
        
        cursorX += glyph.xAdvance;
    }
    else
    {
        cursorX = 0;
        cursorY += gfxFont->yAdvance;
    }
}

void displayPrintf(const char* format, ...)
{
    char buf[64];
    va_list varArgs;
#ifdef DISPLAY_UTF8_SUPPORT
    int prevChar = -1;
#endif

    va_start(varArgs, format);
    vsnprintf(buf, sizeof(buf), format, varArgs);
    va_end(varArgs);

    for(uint8_t i = 0; buf[i]; ++i)
    {
        unsigned char c = buf[i];
        // utf8 support
#ifdef DISPLAY_UTF8_SUPPORT
        switch (prevChar) 
        {
            case 0xD0: 
            {
                if (c == 0x81) { c = 0xA8; break; }
                if (c >= 0x90 && c <= 0xBF) c += 0x30;
                prevChar = -1;
                break;
            }
            case 0xD1: 
            {
                if (c == 0x91) { c = 0xB8; break; }
                if (c >= 0x80 && c <= 0x8F) c += 0x70;
                prevChar = -1;
                break;
            }
            case -1: if(c >= 0xC0) prevChar = c;
        }
        if(prevChar == -1)
#endif
            displayChar(c);
    }
}

void displayPutPixel(int16_t x, int16_t y, uint16_t color)
{
    displayRect(x, y, 1, 1, color);
}

void displayLine(int16_t x1, int16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    if(x1 == x2)
    {
        if(y1 > y2) 
            displayRect(x1, y2, 1, y1 - y2 + 1, color);
        else
            displayRect(x1, y1, 1, y2 - y1 + 1, color);
    }
    else if(y1 == y2)
    {
        if(x1 > x2) 
            displayRect(x2, y1, x1 - x2 + 1, 1, color);
        else
            displayRect(x1, y1, x2 - x1 + 1, 1, color);
    }
    // TODO: add generic line support
}

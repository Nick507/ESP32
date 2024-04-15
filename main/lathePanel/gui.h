#pragma once

#include "display.h"
#include <stdlib.h>

#define PROGMEM   // for fonts

//#include "FreeMono18pt7b.h"
//#include "FreeMonoBold9pt7b.h"
//#include "FreeSansBold9pt7b.h"
#include "FreeSans12pt7b.h"
#include "FixedFont24x32.h"

//#define GUI_BUTTON_DEFAULT_FONT &FreeMonoBold9pt7b
//#define GUI_BUTTON_DEFAULT_FONT &FreeSansBold9pt7b
#define GUI_BUTTON_DEFAULT_FONT &FreeSans12pt7b
#define GUI_CAPTION_DEFAULT_FONT &FreeSans12pt7b
#define GUI_DIGITS_DEFAULT_FONT &FixedFont24x32

struct Event
{
    enum Type {etNone, etPress, etRelease, etMove};
    Type type;
    uint16_t x;
    uint16_t y;
};

//=====================================================================================
//
//                             RectangleObject
//
//=====================================================================================

class RectangleObject
{
protected:
    uint16_t m_x;
    uint16_t m_y;
    uint16_t m_sx;
    uint16_t m_sy;
    bool m_neetToRedraw;

public:
    RectangleObject(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy);
    bool isCursorInside(Event * e);
    virtual void draw() = 0;
    virtual void drawAtPos(uint16_t x, uint16_t y);
    virtual bool handleEvent(Event * e);
    virtual bool isNeedToRedraw();
    virtual void setNeedToRedraw(bool flag);
    void drawFrame(bool pressed, uint16_t x, uint16_t y, uint16_t sx, uint16_t sy);
    void drawFrame(bool pressed);
    void setX(uint16_t x) {m_x = x;}
    void setY(uint16_t y) {m_y = y;}
    void setSX(uint16_t sx) {m_sx = sx;}
    void setSY(uint16_t sy) {m_sy = sy;}
    uint16_t getX() {return m_x;}
    uint16_t getY() {return m_y;}
    uint16_t getSX() {return m_sx;}
    uint16_t getSY() {return m_sy;}
};

enum UserEvent {etNone, etButtonPressed, etButtonReleased, etGrblOk, etGrblError, etValueChanged};

typedef void (* ObjectCallbackPtr) (UserEvent e, RectangleObject * obj);

//=====================================================================================
//
//                             Window
//
//=====================================================================================

class Window : public RectangleObject
{
    RectangleObject ** m_objects;

public:
    Window(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, RectangleObject ** objects);

    void draw() override;
    void setNeedToRedraw(bool flag) override;
    bool handleEvent(Event * e) override;
};

//=====================================================================================
//
//                             Button
//
//=====================================================================================

class Button : public RectangleObject
{
protected:
    const char * m_text;
    bool m_isPressed = false;
    bool m_isChecked = false;
    bool m_enabled = true;
    const GFXfont * m_font;
    ObjectCallbackPtr m_cb;

    void drawPressed();
    void drawReleased();

public:
    Button(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, const char * text, const GFXfont * font, ObjectCallbackPtr cb);
    virtual void draw() override;
    virtual bool handleEvent(Event * e) override;
    void setText(const char * text);
    void setEnabled(bool enabled);
    const char * getText() {return m_text;}
};

//=====================================================================================
//
//                             ToggleButton
//
//=====================================================================================

class ToggleButton : public Button
{
public:
    ToggleButton(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, const char * text, const GFXfont * font, ObjectCallbackPtr cb);
    bool handleEvent(Event * e) override;
    void setState(bool pressed);
    bool getState() {return m_isPressed;}
};

//=====================================================================================
//
//                             Caption
//
//=====================================================================================

class Caption : public RectangleObject
{
    const GFXfont * m_font;
    const char * m_text;

public:
    Caption(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, const char * text, const GFXfont * font = GUI_CAPTION_DEFAULT_FONT);
    void draw() override;
    void setText(const char * text) {m_text = text;}
};

//=====================================================================================
//
//                             NumericValue
//
//=====================================================================================

enum NumericValueType {nvFloat, nvInt32};

class NumericValue : public RectangleObject
{
    NumericValueType m_type;
    union
    {
        float * m_valueFloat;
        int32_t * m_valueInt32;
    };
    
    union 
    {
        float m_prevValueFloat;
        int32_t m_prevValueInt32;
    };
    const char * m_valueFormat;
    const GFXfont * m_font;
    ObjectCallbackPtr m_cb;

public:
    
    NumericValue(uint16_t x, uint16_t y, NumericValueType type, void * value, const char * valueFormat, const GFXfont * font, ObjectCallbackPtr cb);
    void draw() override;
    bool handleEvent(Event * e) override;
    bool isNeedToRedraw() override;
    void setValue(void * value);
    NumericValueType getType() {return m_type;}
    void getStringValue(char * buf, size_t len);
};

//=====================================================================================
//
//                             NamedUnitsValue
//
//=====================================================================================

class NamedUnitsValue : public RectangleObject
{
    Caption m_name;
    NumericValue m_value;
    Caption m_units;

public:
    
    NamedUnitsValue(uint16_t x, uint16_t y, NumericValueType type, void * value, const char * valueFormat, const GFXfont * font, const char * name, const char * units, ObjectCallbackPtr cb);
    void draw() override;
    bool handleEvent(Event * e) override;
    bool isNeedToRedraw() override;
    void setNeedToRedraw(bool flag) override;
    void setValue(void * value) {m_value.setValue(value);}
    NumericValueType getType() {return m_value.getType();}
    Caption & getName() {return m_name;}
    Caption & getUnits() {return m_units;}
    NumericValue & getValue() {return m_value;}
};

//=====================================================================================
//
//                             MultilineText
//
//=====================================================================================

class MultilineText : public RectangleObject
{
    const char * m_text;
    int m_selectedLine;

public:
    MultilineText(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, const char * text);
    void draw() override;
    bool drawLine(int lineNumber);
    bool handleEvent(Event * e) override;
    int getSelectedLineNumber() {return m_selectedLine;}
    const char * getLineText(int lineNumber);
};

//=====================================================================================
//
//                             Global
//
//=====================================================================================

void guiInit(RectangleObject * object);
void guiTask();
void guiChangeWindow(RectangleObject * object);
void guiBackWindow();


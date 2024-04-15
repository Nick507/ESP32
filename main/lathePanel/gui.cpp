#include "gui.h"
#include "display.h"
#include <stdio.h>
#include <strings.h>
#include <cstring>
#include <stdlib.h>

RectangleObject * currentObject = NULL;
RectangleObject * prevObject = NULL;

//=====================================================================================
//
//                             RectangleObject
//
//=====================================================================================

RectangleObject::RectangleObject(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy):
    m_x(x), m_y(y), m_sx(sx), m_sy(sy), m_neetToRedraw(true)
{
}
    
bool RectangleObject::isCursorInside(Event * e)
{
    return (e->x > m_x && e->x < m_x + m_sx && e->y > m_y && e->y < m_y + m_sy);
}

bool RectangleObject::isNeedToRedraw()
{
    return m_neetToRedraw;
}

void RectangleObject::setNeedToRedraw(bool flag)
{
    m_neetToRedraw = flag;
}

void RectangleObject::drawFrame(bool pressed, uint16_t x, uint16_t y, uint16_t sx, uint16_t sy)
{
    displayLine(x, y, x + sx - 2, y, pressed ? TFT_BLACK : TFT_WHITE);
    displayLine(x, y + 1, x, y + sy - 2, pressed ? TFT_BLACK : TFT_WHITE);
    displayLine(x + 1, y + 1, x + sx - 3, y + 1, pressed ? TFT_GRAY_128 : TFT_GRAY_224);
    displayLine(x + 1, y + 2, x + 1, y + sy - 3, pressed ? TFT_GRAY_128 : TFT_GRAY_224);
    displayLine(x + 1, y + sy - 2, x + sx - 3, y + sy - 2, pressed ? TFT_GRAY_224 : TFT_GRAY_128);
    displayLine(x + sx - 2, y + 1, x + sx - 2, y + sy - 2, pressed ? TFT_GRAY_224 : TFT_GRAY_128);
    displayLine(x, y + sy - 1, x + sx - 2, y + sy - 1, pressed ? TFT_WHITE : TFT_BLACK);
    displayLine(x + sx - 1, y, x + sx - 1, y + sy - 1, pressed ? TFT_WHITE : TFT_BLACK);
}

void RectangleObject::drawFrame(bool pressed)
{
    drawFrame(pressed, m_x, m_y, m_sx, m_sy);
}

bool RectangleObject::handleEvent(Event * e)
{
    return false;
}

void RectangleObject::drawAtPos(uint16_t x, uint16_t y)
{
    uint16_t px = m_x, py = m_y;
    m_x = x;
    m_y = y;
    draw();
    m_x = px;
    m_y = py;
}

//=====================================================================================
//
//                             Window
//
//=====================================================================================

Window::Window(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, RectangleObject ** objects) :
    RectangleObject(x, y, sx, sy), m_objects(objects)
{
}

void Window::draw()
{
    if(isNeedToRedraw())
    {
        drawFrame(false);
        displayRect(m_x + 2, m_y + 2, m_sx - 4, m_sy - 4, TFT_GRAY_192);
        RectangleObject::setNeedToRedraw(false);
    }

    for(int i = 0; m_objects[i]; ++i)
    {
        if(m_objects[i]->isNeedToRedraw())
        {
            m_objects[i]->draw();
            m_objects[i]->setNeedToRedraw(false);
        } 
    }
}

void Window::setNeedToRedraw(bool flag)
{
    RectangleObject::setNeedToRedraw(flag);
    for(int i = 0; m_objects[i]; ++i)
    {
        m_objects[i]->setNeedToRedraw(flag);
    }
}

bool Window::handleEvent(Event * event)
{
    for(int i = 0; m_objects[i]; ++i)
    {
        m_objects[i]->handleEvent(event);
    }
    return true;
}

//=====================================================================================
//
//                             Button
//
//=====================================================================================

Button::Button(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, const char * text, const GFXfont * font, ObjectCallbackPtr cb) :
    RectangleObject(x, y, sx, sy), m_text(text), m_font(font), m_cb(cb)
{
}

void Button::drawPressed()
{
    drawFrame(true);
    displayRect(m_x + 2, m_y + 2, m_sx - 4, m_sy - 4, TFT_GRAY_192);
    uint16_t tsx, tsy;
    int16_t tyoff;
    displaySetTextFont(m_font);
    displayGetTextBounds(m_text, &tsx, &tsy, &tyoff);
    displaySetCursor(m_x + (m_sx - tsx) / 2 + 2, m_y + (m_sy - tsy) / 2 + 2 + tsy - tyoff);
    displaySetTextColor(m_enabled ? TFT_BLACK : TFT_GRAY_224, TFT_GRAY_192);
    displayPrintf(m_text);
}

void Button::drawReleased()
{
    drawFrame(false);
    displayRect(m_x + 2, m_y + 2, m_sx - 4, m_sy - 4, TFT_GRAY_192);
    uint16_t tsx, tsy;
    int16_t tyoff;
    displaySetTextFont(m_font);
    displayGetTextBounds(m_text, &tsx, &tsy, &tyoff);
    displaySetCursor(m_x + (m_sx - tsx) / 2, m_y + (m_sy - tsy) / 2 + tsy - tyoff);
    displaySetTextColor(m_enabled ? TFT_BLACK : TFT_GRAY_224, TFT_GRAY_192);
    displayPrintf(m_text);
}

void Button::draw()
{
    if(m_isPressed) drawPressed(); else drawReleased();
}

void Button::setText(const char * text)
{
    m_text = text;
    setNeedToRedraw(true);
}

void Button::setEnabled(bool enabled)
{
    m_enabled = enabled;
    setNeedToRedraw(true);
}

bool Button::handleEvent(Event * e)
{
    if(!m_enabled) return false;

    switch(e->type)
    {
        case Event::etMove:
            if(m_isChecked && !m_isPressed && isCursorInside(e))
            {
                m_isPressed = true;
                setNeedToRedraw(true);
                break;
            }
            if(m_isPressed && !isCursorInside(e))
            {
                m_isPressed = false;
                setNeedToRedraw(true);
            }
            break;

        case Event::etPress:
            if(!m_isPressed && isCursorInside(e))
            {
                m_isPressed = true;
                m_isChecked = true;
                setNeedToRedraw(true);
            }
            break;

        case Event::etRelease:
            if(m_isChecked && isCursorInside(e) && m_cb) m_cb(etButtonPressed, this);
            
            m_isChecked = false;

            if(m_isPressed)
            {
                m_isPressed = false;
                setNeedToRedraw(true);
            }
            break;

        default:
            break;
    }
    return true;
}

//=====================================================================================
//
//                             ToggleButton
//
//=====================================================================================

ToggleButton::ToggleButton(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, const char * text, const GFXfont * font, ObjectCallbackPtr cb) :
    Button(x, y, sx, sy, text, font, cb)
{

}

bool ToggleButton::handleEvent(Event * e)
{
    if(!m_enabled) return true;

    switch(e->type)
    {
        case Event::etMove:
            break;

        case Event::etPress:
            if(isCursorInside(e))
            {
                m_isPressed ^= true;
                if(m_cb) m_cb(m_isPressed ? etButtonPressed : etButtonReleased, this);
                setNeedToRedraw(true);
            }
            break;

        case Event::etRelease:
            break;

        default:
            break;
    }
    return true;

}

void ToggleButton::setState(bool pressed)
{
    if(m_isPressed != pressed)
    {
        m_isPressed = pressed;
        setNeedToRedraw(true);
    }

}

//=====================================================================================
//
//                             Caption
//
//=====================================================================================

Caption::Caption(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, const char * text, const GFXfont * font):
    RectangleObject(x, y, sx, sy), m_font(font), m_text(text)
{
    if(!m_text) return;
    if(sx == 0 && sy == 0)
    {
        displaySetTextFont(m_font);
        displayGetTextBounds(m_text, &m_sx, &m_sy, NULL);
    }
}

void Caption::draw()
{
    if(!m_text) return;
    displayRect(m_x, m_y, m_sx, m_sy, TFT_GRAY_192); // to be able to display dynamic captions
    displaySetTextColor(TFT_BLACK, TFT_GRAY_192);
    displaySetTextFont(m_font);
    uint16_t tsx, tsy;
    int16_t tyoff;
    displayGetTextBounds(m_text, &tsx, &tsy, &tyoff);
    displaySetCursor(m_x, m_y + (m_sy - tsy) / 2 + tsy - tyoff);
    displayPrintf(m_text);
}

//=====================================================================================
//
//                             NumericValueEditor
//
//=====================================================================================

class NumericValueEditor : public RectangleObject
{
    static char m_buf[16];
    static NamedUnitsValue * m_value;

public:
    NumericValueEditor(uint16_t x, uint16_t y);
    void draw() override;

    static void bindValue(NamedUnitsValue * value);
    static void digitsButtonCB(UserEvent event, RectangleObject * obj);
    static void cancelButtonCB(UserEvent event, RectangleObject * obj);
    static void delButtonCB(UserEvent event, RectangleObject * obj);
    static void okButtonCB(UserEvent event, RectangleObject * obj);
};

char NumericValueEditor::m_buf[16];
NamedUnitsValue * NumericValueEditor::m_value = NULL;


#define NVE_BUTTON_X 30
#define NVE_BUTTON_Y 120
#define NVE_BUTTON_SX 60
#define NVE_BUTTON_SY 40
#define NVE_BUTTON_STEPX 90
#define NVE_BUTTON_STEPY 60


Button nveButton1(NVE_BUTTON_X + NVE_BUTTON_STEPX * 0, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 0, NVE_BUTTON_SX, NVE_BUTTON_SY, "1", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::digitsButtonCB);
Button nveButton2(NVE_BUTTON_X + NVE_BUTTON_STEPX * 1, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 0, NVE_BUTTON_SX, NVE_BUTTON_SY, "2", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::digitsButtonCB);
Button nveButton3(NVE_BUTTON_X + NVE_BUTTON_STEPX * 2, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 0, NVE_BUTTON_SX, NVE_BUTTON_SY, "3", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::digitsButtonCB);
Button nveButton4(NVE_BUTTON_X + NVE_BUTTON_STEPX * 3, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 0, NVE_BUTTON_SX, NVE_BUTTON_SY, "4", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::digitsButtonCB);
Button nveButton5(NVE_BUTTON_X + NVE_BUTTON_STEPX * 4, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 0, NVE_BUTTON_SX, NVE_BUTTON_SY, "5", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::digitsButtonCB);
Button nveButton6(NVE_BUTTON_X + NVE_BUTTON_STEPX * 0, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 1, NVE_BUTTON_SX, NVE_BUTTON_SY, "6", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::digitsButtonCB);
Button nveButton7(NVE_BUTTON_X + NVE_BUTTON_STEPX * 1, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 1, NVE_BUTTON_SX, NVE_BUTTON_SY, "7", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::digitsButtonCB);
Button nveButton8(NVE_BUTTON_X + NVE_BUTTON_STEPX * 2, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 1, NVE_BUTTON_SX, NVE_BUTTON_SY, "8", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::digitsButtonCB);
Button nveButton9(NVE_BUTTON_X + NVE_BUTTON_STEPX * 3, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 1, NVE_BUTTON_SX, NVE_BUTTON_SY, "9", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::digitsButtonCB);
Button nveButton0(NVE_BUTTON_X + NVE_BUTTON_STEPX * 4, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 1, NVE_BUTTON_SX, NVE_BUTTON_SY, "0", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::digitsButtonCB);
Button nveButtonMinus(NVE_BUTTON_X + NVE_BUTTON_STEPX * 0, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 2, NVE_BUTTON_SX, NVE_BUTTON_SY, "-", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::digitsButtonCB);
Button nveButtonPoint(NVE_BUTTON_X + NVE_BUTTON_STEPX * 1, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 2, NVE_BUTTON_SX, NVE_BUTTON_SY, ".", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::digitsButtonCB);
Button nveButtonOk(NVE_BUTTON_X + NVE_BUTTON_STEPX * 2, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 2, NVE_BUTTON_SX, NVE_BUTTON_SY, "Ok", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::okButtonCB);
Button nveButtonDel(NVE_BUTTON_X + NVE_BUTTON_STEPX * 3, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 2, NVE_BUTTON_SX, NVE_BUTTON_SY, "Del", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::delButtonCB);
Button nveButtonCancel(NVE_BUTTON_X + NVE_BUTTON_STEPX * 4, NVE_BUTTON_Y + NVE_BUTTON_STEPY * 2, NVE_BUTTON_SX, NVE_BUTTON_SY, "Back", GUI_BUTTON_DEFAULT_FONT, &NumericValueEditor::cancelButtonCB);

NumericValueEditor nveField(150, 70);


NumericValueEditor::NumericValueEditor(uint16_t x, uint16_t y) :
    RectangleObject(x, y, 0, 0)
{
    m_buf[0] = 0;
    displaySetTextFont(GUI_DIGITS_DEFAULT_FONT);
    displayGetTextBounds("%8s", &m_sx, &m_sy, NULL, m_buf);
    m_sx += 4;
    m_sy += 4;
}

void NumericValueEditor::draw()
{
    m_x = m_value->getName().getSX() + 5 + (DISPLAY_WIDTH - (m_sx + 10 + m_value->getName().getSX() + m_value->getUnits().getSX())) / 2;

    drawFrame(true);
    displaySetTextFont(GUI_DIGITS_DEFAULT_FONT);
    displaySetCursor(m_x + 2, m_y + m_sy - 2);
    displaySetTextColor(TFT_BLACK, TFT_WHITE);
    displayPrintf("%8s", m_buf);
    
    m_value->getUnits().drawAtPos(m_x + m_sx + 4/*frame*/ + 5/*space*/, m_y);
    m_value->getName().drawAtPos(m_x - m_value->getName().getSX() - 5/*space*/, m_y);
}

void NumericValueEditor::bindValue(NamedUnitsValue * value) 
{
    m_value = value;
    m_value->getValue().getStringValue(m_buf, sizeof(m_buf));

    // trim value, because it can be formatted with spaces
    int i = strcspn(m_buf, "-.0123456789");
    memmove(m_buf, m_buf + i, sizeof(m_buf) - i);
}

void NumericValueEditor::digitsButtonCB(UserEvent event, RectangleObject * obj)
{
    char c = *(((Button*)obj)->getText());

    if(strlen(m_buf) > 7) return;
    if(c == '-' && m_buf[0]) return;
    if(c == '0' && m_buf[0] == '0' && m_buf[1] == 0) return;
    if(c == '.' && strchr(m_buf, '.')) return;
    
    strcat(m_buf, ((Button*)obj)->getText());
    nveField.setNeedToRedraw(true);
}

void NumericValueEditor::delButtonCB(UserEvent event, RectangleObject * obj)
{
    auto l = strlen(m_buf);
    if(l > 0) m_buf[l - 1] = 0;
    nveField.setNeedToRedraw(true);
}

void NumericValueEditor::okButtonCB(UserEvent event, RectangleObject * obj)
{
    switch(m_value->getType())
    {
        case nvFloat: 
        {
            float v = atof(m_buf);
            m_value->setValue(&v);
        }
        break;
        case nvInt32:
        {
            int32_t v = atoi(m_buf);
            m_value->setValue(&v);
        }
        break;
    }
    
    guiBackWindow();
}

void NumericValueEditor::cancelButtonCB(UserEvent event, RectangleObject * obj)
{
    guiBackWindow();
}

RectangleObject * numericValueEditorObjects[] = {&nveField, &nveButton1, &nveButton2, &nveButton3, &nveButton4, &nveButton5, 
                                  &nveButton6, &nveButton7, &nveButton8, &nveButton9, &nveButtonMinus, 
                                  &nveButton0, &nveButtonPoint, &nveButtonOk, &nveButtonDel, &nveButtonCancel, 
                                  NULL};

Window numericValueEditorWindow(10, 50, DISPLAY_WIDTH - 20, 250, numericValueEditorObjects);

//=====================================================================================
//
//                             NumericValue
//
//=====================================================================================

NumericValue::NumericValue(uint16_t x, uint16_t y, NumericValueType type, void * value, const char * valueFormat, const GFXfont * font, ObjectCallbackPtr cb) :
    RectangleObject(x, y, 0, 0), m_type(type), m_valueFormat(valueFormat), m_font(font), m_cb(cb)
{
    uint16_t sx, sy;

    displaySetTextFont(m_font);

    switch(m_type)
    {
        case nvFloat:
            m_valueFloat = (float*)value; 
            m_prevValueFloat = *m_valueFloat; 
            displayGetTextBounds(m_valueFormat, &sx, &sy, NULL, *m_valueFloat);
            break;
        case nvInt32: 
            m_valueInt32 = (int32_t*)value; 
            m_prevValueInt32 = *m_valueInt32; 
            displayGetTextBounds(m_valueFormat, &sx, &sy, NULL, *m_valueInt32);
            break;
    }
    m_sx = sx + 4;
    m_sy = sy + 4;
}

void NumericValue::draw()
{
    drawFrame(true);

    displaySetTextFont(m_font);
    displaySetCursor(m_x + 2, m_y + m_sy - 2);
    displaySetTextColor(TFT_BLACK, TFT_WHITE);

    switch(m_type)
    {
        case nvFloat:
            m_prevValueFloat = *m_valueFloat; 
            displayPrintf(m_valueFormat, m_prevValueFloat); 
            break;
        case nvInt32:
            m_prevValueInt32 = *m_valueInt32; 
            displayPrintf(m_valueFormat, m_prevValueInt32); 
            break;
    }
}

void NumericValue::getStringValue(char * buf, size_t len)
{
    switch(m_type)
    {
        case nvFloat: snprintf(buf, len, m_valueFormat, *m_valueFloat); break;
        case nvInt32: snprintf(buf, len, m_valueFormat, *m_valueInt32); break;
    }
}

bool NumericValue::isNeedToRedraw()
{
    switch(m_type)
    {
        case nvFloat:
            if(*m_valueFloat != m_prevValueFloat) return true;
            break;
        case nvInt32:
            if(*m_valueInt32 != m_prevValueInt32) return true;
            break;
    }
    return RectangleObject::isNeedToRedraw();
}

bool NumericValue::handleEvent(Event * e)
{
    return (e->type == Event::etPress && isCursorInside(e));
}

void NumericValue::setValue(void * value)
{
    switch(m_type)
    {
        case nvFloat: *m_valueFloat = *(float*)value; break;
        case nvInt32: *m_valueInt32 = *(int32_t*)value; break;
    }

    if(m_cb) m_cb(etValueChanged, this);
}

//=====================================================================================
//
//                             NamedUnitsValue
//
//=====================================================================================

NamedUnitsValue::NamedUnitsValue(uint16_t x, uint16_t y, NumericValueType type, void * value, const char * valueFormat, const GFXfont * font, const char * name, const char * units, ObjectCallbackPtr cb) :
    RectangleObject(x, y, 0, 0), 
    m_name(x, y, 0, 0, name, font),
    m_value(x, y, type, value, valueFormat, GUI_DIGITS_DEFAULT_FONT, cb),
    m_units(x, y, 0, 0, units, font)
{
    m_name.setSY(m_value.getSY());
    m_units.setSY(m_value.getSY());
    
    m_value.setX(m_name.getX() + m_name.getSX() + 5);
    m_units.setX(m_value.getX() + m_value.getSX() + 5);
}

void NamedUnitsValue::draw()
{
    if(m_name.isNeedToRedraw()) {m_name.draw(); m_name.setNeedToRedraw(false);}
    if(m_value.isNeedToRedraw()) {m_value.draw(); m_value.setNeedToRedraw(false);}
    if(m_units.isNeedToRedraw()) {m_units.draw(); m_units.setNeedToRedraw(false);}
}

void NamedUnitsValue::setNeedToRedraw(bool flag)
{
    m_name.setNeedToRedraw(flag);
    m_value.setNeedToRedraw(flag);
    m_units.setNeedToRedraw(flag);
}

bool NamedUnitsValue::handleEvent(Event * e)
{
    if(m_value.handleEvent(e))
    {
        NumericValueEditor::bindValue(this);
        guiChangeWindow(&numericValueEditorWindow);
    }
    return true;
}

bool NamedUnitsValue::isNeedToRedraw()
{
    return m_name.isNeedToRedraw() || m_value.isNeedToRedraw() || m_units.isNeedToRedraw();
}

//=====================================================================================
//
//                             MultilineText
//
//=====================================================================================

MultilineText::MultilineText(uint16_t x, uint16_t y, uint16_t sx, uint16_t sy, const char * text) :
    RectangleObject(x, y, sx, sy),
    m_text(text),
    m_selectedLine(-1)
{
}

void MultilineText::draw()
{
    drawFrame(true);
    displayRect(m_x + 2, m_y + 2, m_sx - 4, m_sy - 4, TFT_WHITE);
    for(int i = 0; drawLine(i); i++);
}

bool MultilineText::drawLine(int lineNumber)
{
    displaySetTextFont(GUI_CAPTION_DEFAULT_FONT);
    
    const char * line = getLineText(lineNumber);
    if(!line) return false;

    int lineHeight = displayGetFontHeight();
    int y = m_y + 2 + lineHeight * lineNumber;

    displayRect(m_x + 2, y, m_sx - 4, lineHeight, lineNumber == m_selectedLine ? TFT_BLUE : TFT_WHITE);
    displaySetTextColor(lineNumber == m_selectedLine ? TFT_WHITE : TFT_BLACK, lineNumber == m_selectedLine ? TFT_BLUE : TFT_WHITE); 
    
    uint16_t sx, sy;
    int16_t tyoff;
    displayGetTextBounds(line, &sx, &sy, &tyoff);
    displaySetCursor(m_x + 2, y + lineHeight - (lineHeight - sy) / 2 - tyoff);
    displayPrintf(line);

    return true;
}

bool MultilineText::handleEvent(Event * e)
{
    if((e->type == Event::etPress || e->type == Event::etMove)  && isCursorInside(e))
    {
        displaySetTextFont(GUI_CAPTION_DEFAULT_FONT);

        int line = (e->y - m_y - 2) / displayGetFontHeight();
        if(getLineText(line) && (line != m_selectedLine))
        {
            int prevLine = m_selectedLine;
            m_selectedLine = line;
            drawLine(m_selectedLine);
            drawLine(prevLine);
            //setNeedToRedraw(true);
        }
    }

    return true;
}

const char * MultilineText::getLineText(int lineNumber)
{
    const char * res = m_text;

    if(lineNumber < 0) return NULL;

    while(*res && lineNumber > 0)
    {
        if(*(++res) == 0) {lineNumber--; res++;}
    }
    return *res ? res : NULL;
}

//=====================================================================================
//
//                             Global
//
//=====================================================================================

void guiChangeWindow(RectangleObject * object)
{
    if(object == currentObject) return;

    prevObject = currentObject;
    currentObject = object;
    currentObject->setNeedToRedraw(true);
    currentObject->draw();
}

void guiBackWindow()
{
    if(prevObject)
    {
        guiChangeWindow(prevObject);
        prevObject = NULL;
    }
}

void guiInit(RectangleObject * object)
{
    displayInit();
    guiChangeWindow(object);
}

void guiTask()
{
    int16_t x = 0;
    int16_t y = 0;
    static bool pressed = false;
    static Event event;
    event.type = Event::etNone;

    if(displayReadTouch(&x, &y))
    {
        if(!pressed)
        {
            pressed = true;
            event.type = Event::etPress;
        }
        else if(x != event.x || y != event.y)
        {
            event.type = Event::etMove;
        }
        event.x = x;
        event.y = y;
    }
    else
    {
        if(pressed)
        {
            pressed = false;
            event.type = Event::etRelease;
        }
    }

    if(event.type != Event::etNone)
    {
        //printf("event=%d x=%d y=%d\n", event.type, event.x, event.y);
        currentObject->handleEvent(&event);
        //displayPutPixel(event.x, event.y, TFT_RED);
    } 
    
    currentObject->draw();
    
}

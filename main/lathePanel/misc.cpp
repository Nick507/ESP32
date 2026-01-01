#include "grblConnect.h"
#include "misc.h"
#include "controller.h"
#include <stdio.h>
#include <string.h>

//=====================================================================================
//
//                      MISC SCREEN      
//
//=====================================================================================

void enaButtonCB(UserEvent event, RectangleObject * obj)
{
    setMotorState(0);
}

void disButtonCB(UserEvent event, RectangleObject * obj)
{
    setMotorState(1);
}

void resetButtonCB(UserEvent event, RectangleObject * obj)
{
    grblRestart();
}

void rigidButtonCB(UserEvent event, RectangleObject * obj);

void miscBackButtonCB(UserEvent event, RectangleObject * obj)
{
    guiChangeWindow(&mainWindow);
}

char captionIPAddr[16] = {"0.0.0.0"};

Button enaButton(10, 10, 90, 50, "Enable", GUI_BUTTON_DEFAULT_FONT, enaButtonCB);
Button disButton(120, 10, 90, 50, "Disable", GUI_BUTTON_DEFAULT_FONT, disButtonCB);
Button resetButton(230, 10, 90, 50, "Reset", GUI_BUTTON_DEFAULT_FONT, resetButtonCB);
Button rigidButton(340, 10, 90, 50, "Rigid", GUI_BUTTON_DEFAULT_FONT, rigidButtonCB);
Caption ipAddr(10, 270, 150, 50, captionIPAddr, GUI_BUTTON_DEFAULT_FONT);
Button miscBackButton(370, 260, 100, 50, "Back", GUI_BUTTON_DEFAULT_FONT, &miscBackButtonCB);
int32_t currentAngle = 0;
NamedUnitsValue currentAngleNUV (10, 70, nvInt32, &currentAngle, "%6d", GUI_BUTTON_DEFAULT_FONT, "Angle", "deg", NULL);

RectangleObject * miscWindowObjects[] = {&enaButton, &disButton, &resetButton, &miscBackButton, &rigidButton, &ipAddr, &currentAngleNUV, NULL};

Window miscWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, miscWindowObjects);

void miscButtonCB(UserEvent event, RectangleObject * obj)
{
    strcpy(captionIPAddr, grblGetIP());
    guiChangeWindow(&miscWindow);
}

void rigidButtonCB(UserEvent event, RectangleObject * obj)
{
    static uint16_t r5, r6, r21;

    if(*rigidButton.getText() == 'R')
    {
        if(!grblModbusWriteReg(5, 250, &r5)   ||
           !grblModbusWriteReg(6, 10, &r6)    ||
           !grblModbusWriteReg(21, 120, &r21) ||
           !grblModbusWriteReg(98, 1, NULL))
        {
            printf("Failed to set modbus registers\n");
            return;
        }
        rigidButton.setText("Soft");
    }
    else
    {
        if(!grblModbusWriteReg(5, r5, NULL) ||
           !grblModbusWriteReg(6, r6, NULL) ||
           !grblModbusWriteReg(21, r21, NULL) ||
           !grblModbusWriteReg(98, 0, NULL))
        {
            printf("Failed to restore modbus registers\n");
        }
        rigidButton.setText("Rigid");
    }
}


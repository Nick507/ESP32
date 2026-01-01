#include "controller.h"
#include "gui.h"
#include "grblConnect.h"
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <cstdarg>
#include <cassert>
#include "macro.h"
#include "misc.h"

// espefuse.py -p COM16 set_flash_voltage 3.3V

ObjectCallbackPtr commandsStack[10];
uint8_t commandsStackSize = 0;

static bool grblCommandInProgress = false;

bool grblExecuteCommand(ObjectCallbackPtr cb, const char* format, ...)
{
    static char buf[128];

    if(grblCommandInProgress)
    {
        printf("grblCommand execute is in progress!\n");
        //assert(false);
        //return false;
    }

    va_list varArgs;
    va_start(varArgs, format);
    vsnprintf(buf, sizeof(buf), format, varArgs);
    va_end(varArgs);

    bool res = grblExecuteCommand(buf);

    if(res)
    {
        if(buf[0]) grblCommandInProgress = true; // if not a test command

        if(cb)
        {
            assert(commandsStackSize < sizeof(commandsStack) / sizeof(ObjectCallbackPtr));
            commandsStack[commandsStackSize++] = cb;
        }
    }
    return res;
}

float grblGetXPosition(void) {return grblXPosition;}
float grblGetYPosition(void) {return grblYPosition;}
float grblGetZPosition(void) {return grblZPosition;}

//=====================================================================================
//
//                             Positions
//
//=====================================================================================

NamedUnitsValue xPositionCaption( 5, 5, nvFloat, &grblXPosition, "%8.3f", GUI_DIGITS_DEFAULT_FONT, "X", NULL, NULL);
NamedUnitsValue yPositionCaption(5, 50, nvFloat, &grblYPosition, "%8.3f", GUI_DIGITS_DEFAULT_FONT, "Y", NULL, NULL);
NamedUnitsValue zPositionCaption(5, 95, nvFloat, &grblZPosition, "%8.3f", GUI_DIGITS_DEFAULT_FONT, "Z", NULL, NULL);

void zeroXPositionButtonCB(UserEvent event, RectangleObject * obj)
{
    grblExecuteCommand(NULL, "G92X0\n");
}

void zeroYPositionButtonCB(UserEvent event, RectangleObject * obj)
{
    grblExecuteCommand(NULL, "G92Y0\n");
}

void zeroZPositionButtonCB(UserEvent event, RectangleObject * obj)
{
    grblExecuteCommand(NULL, "G92Z0\n");
}

Button zeroXPositionButton(235, 5, 36, 36, "0", GUI_DIGITS_DEFAULT_FONT, &zeroXPositionButtonCB);
Button zeroYPositionButton(235, 50, 36, 36, "0", GUI_DIGITS_DEFAULT_FONT, &zeroYPositionButtonCB);
Button zeroZPositionButton(235, 95, 36, 36, "0", GUI_DIGITS_DEFAULT_FONT, &zeroZPositionButtonCB);

//=====================================================================================
//
//                             Spindle control
//
//=====================================================================================

uint32_t spindleSpeed = 1000;
SpindleState spindleState = ssStopped;
bool spindleDirectionForward = true;

void spindleToggleButtonCB(UserEvent event, RectangleObject * obj);
void spindleIncreaseSpeedButtonCB(UserEvent event, RectangleObject * obj);
void spindleDecreaseSpeedButtonCB(UserEvent event, RectangleObject * obj);
void spindleDirButtonCB(UserEvent event, RectangleObject * obj);

NamedUnitsValue spindleSpeedCaption(280, 140, nvInt32, &spindleSpeed, "%4d", GUI_BUTTON_DEFAULT_FONT, NULL, "RPM", NULL);
Button spindleToggleButton(280, 260, 80, 50, "Run", GUI_BUTTON_DEFAULT_FONT, &spindleToggleButtonCB);
Button spindleDirButton(380, 260, 80, 50, "Rev", GUI_BUTTON_DEFAULT_FONT, &spindleDirButtonCB);
Button spindleDecreaseSpeedButton(280, 190, 80, 50, "-", GUI_BUTTON_DEFAULT_FONT, &spindleDecreaseSpeedButtonCB);
Button spindleIncreaseSpeedButton(380, 190, 80, 50, "+", GUI_BUTTON_DEFAULT_FONT, &spindleIncreaseSpeedButtonCB);

// void mainWindow()
// {
//     if(cb)
//     {
//         spindleToggleButton.setEnabled(false);
//         spindleIncreaseSpeedButton.setEnabled(false);
//         spindleDecreaseSpeedButton.setEnabled(false);
//         spindleDirButton.setEnabled(false);
//         jogYAxisButton.setEnabled(false);
//         if(jogYAxisButton.getState())
//         {
//             jogCurrentAxis = NULL;
//             jogYAxisButton.setState(false);
//         }
//         threadWindowButton.setEnabled(false);
//     }
//     else
//     {
//         spindleToggleButton.setEnabled(true);
//         spindleIncreaseSpeedButton.setEnabled(true);
//         spindleDecreaseSpeedButton.setEnabled(true);
//         spindleDirButton.setEnabled(!spindleRunning);
//         jogYAxisButton.setEnabled(!spindleRunning);
//         threadWindowButton.setEnabled(!spindleRunning);
//     }
// }

void spindleToggleButtonCB(UserEvent event, RectangleObject * obj)
{
    if(event == etButtonPressed)
    {
        switch(spindleState)
        {
            case ssRun:
                if(grblExecuteCommand(spindleToggleButtonCB, "M%c S1\n", spindleDirectionForward ? '3' : '4')) spindleState = ssDecelerating;
                break;
            case ssStopped:
                if(grblExecuteCommand(spindleToggleButtonCB, "M%c S%d\n", spindleDirectionForward ? '3' : '4', spindleSpeed)) spindleState = ssRunning;
                break;
            default: 
                break;
        }
    }
    else if(event == etGrblOk)
    {
        switch(spindleState)
        {
            case ssDecelerating:
                if(grblExecuteCommand(spindleToggleButtonCB, "M5\n")) spindleState = ssStopping; else spindleState = ssRun;
                break;
            case ssStopping:
                spindleToggleButton.setText("Run");
                spindleState = ssStopped;
                break;
            case ssRunning:
                spindleToggleButton.setText("Stop");
                spindleState = ssRun;
                break;
            default: 
                break;
        }
    }
    else if(event == etGrblError)
    {
        switch(spindleState)
        {
            case ssDecelerating:
            case ssStopping:
                spindleState = ssRun;
                break;
            case ssRunning:
                spindleState = ssStopped;
                break;
            default: 
                break;
        }
    }
}

void spindleIncreaseSpeedButtonCB(UserEvent event, RectangleObject * obj)
{
    if(event == etButtonPressed)
    {
        if(spindleSpeed >= 3000) return;

        spindleSpeed += 100;
        if(spindleState == ssRun)
        {
            grblExecuteCommand(spindleIncreaseSpeedButtonCB, "M%c S%d\n", spindleDirectionForward ? '3' : '4', spindleSpeed);
            //spindleCommandLock();
        }
    }
    else if(event == etGrblOk)
    {
        //spindleCommandLock(NULL);
    }
    else if(event == etGrblError)
    {
        spindleSpeed -= 100;
        //spindleCommandLock(NULL);
    }
}

void spindleDecreaseSpeedButtonCB(UserEvent event, RectangleObject * obj)
{
    if(event == etButtonPressed)
    {
        if(spindleSpeed <= 100) return;

        spindleSpeed -= 100;

        if(spindleState == ssRun)
        {
            grblExecuteCommand(spindleDecreaseSpeedButtonCB, "M%c S%d\n", spindleDirectionForward ? '3' : '4', spindleSpeed);
            //spindleCommandLock();
        }
    }
    else if(event == etGrblOk)
    {
        //spindleCommandLock(NULL);
    }
    else if(event == etGrblError)
    {
        spindleSpeed += 100;
        //spindleCommandLock(NULL);
    }
}

void spindleDirButtonCB(UserEvent event, RectangleObject * obj)
{
    spindleDirectionForward ^= true;
    spindleDirButton.setText(spindleDirectionForward ? "Rev" : "Fwd");
}

// Helper functions for external modules
void setSpindleSpeed(uint32_t speed)
{
    spindleSpeed = speed;
}

SpindleState getSpindleState(void)
{
    return spindleState;
}

void spindleToggle(void)
{
    spindleToggleButtonCB(etButtonPressed, NULL);
}

//=====================================================================================
//
//                            Jog
//
//=====================================================================================

const char * jogCurrentAxis = NULL;
uint16_t jogCurrentDivider = 0;

void jogAxisButtonCB(UserEvent event, RectangleObject * obj);

ToggleButton jogXAxisButton(300, 10, 50, 50, "X", GUI_DIGITS_DEFAULT_FONT, jogAxisButtonCB);
ToggleButton jogYAxisButton(360, 10, 50, 50, "Y", GUI_DIGITS_DEFAULT_FONT, jogAxisButtonCB);
ToggleButton jogZAxisButton(420, 10, 50, 50, "Z", GUI_DIGITS_DEFAULT_FONT, jogAxisButtonCB);

void jogAxisButtonCB(UserEvent event, RectangleObject * obj)
{
    if(event == etButtonReleased) jogCurrentAxis = NULL;
    if(event == etButtonPressed)
    {
        if(obj != &jogXAxisButton) jogXAxisButton.setState(false);
        if(obj != &jogYAxisButton) jogYAxisButton.setState(false);
        if(obj != &jogZAxisButton) jogZAxisButton.setState(false);

        jogCurrentAxis = ((ToggleButton*)obj)->getText();
    }
}

void jogStepButtonCB(UserEvent event, RectangleObject * obj);

ToggleButton jogStep1Button(300, 70, 50, 50, "1", GUI_BUTTON_DEFAULT_FONT, jogStepButtonCB);
ToggleButton jogStep01Button(360, 70, 50, 50, ".1", GUI_BUTTON_DEFAULT_FONT, jogStepButtonCB);
ToggleButton jogStep001Button(420, 70, 50, 50, ".01", GUI_BUTTON_DEFAULT_FONT, jogStepButtonCB);

void jogStepButtonCB(UserEvent event, RectangleObject * obj)
{
    if(event == etButtonReleased) jogCurrentDivider = 0; //((ToggleButton*)obj)->setState(true);
    if(event == etButtonPressed)
    {
        if(obj != &jogStep1Button) jogStep1Button.setState(false);
        if(obj != &jogStep01Button) jogStep01Button.setState(false);
        if(obj != &jogStep001Button) jogStep001Button.setState(false);

        if(obj == &jogStep1Button) jogCurrentDivider = 1;
        if(obj == &jogStep01Button) jogCurrentDivider = 10;
        if(obj == &jogStep001Button) jogCurrentDivider = 100;
    }
}




//=====================================================================================
//
//                            MAIN SCREEN
//
//=====================================================================================

int16_t grblState = 0;
Caption grblStateValue(70, 260, 70, 50, "Idle", GUI_BUTTON_DEFAULT_FONT);

void holdButtonCB(UserEvent event, RectangleObject * obj);
Button holdButton(10, 260, 50, 50, "||", GUI_BUTTON_DEFAULT_FONT, holdButtonCB);

void holdButtonCB(UserEvent event, RectangleObject * obj)
{
    if(*holdButton.getText() == '>')
    {
        grblEnqueueRealtimeCommand('~');
        holdButton.setText("||");
    }
    else if(*holdButton.getText() == 'C')
    {
        grblExecuteCommand(NULL, "$X");
        holdButton.setText("||");
    }
    else
    {
        grblEnqueueRealtimeCommand('!');
        holdButton.setText(">");
    }
}

Button macroButton(10, 140, 100, 50, "Macro", GUI_BUTTON_DEFAULT_FONT, macroWindowButtonCB);
Button miscButton(120, 140, 70, 50, "Misc", GUI_BUTTON_DEFAULT_FONT, miscButtonCB);

RectangleObject * mainWindowObjects[] = {&xPositionCaption, &yPositionCaption, &zPositionCaption, 
                                  &zeroXPositionButton, &zeroYPositionButton, &zeroZPositionButton, 
                                  &jogXAxisButton, &jogYAxisButton, &jogZAxisButton, 
                                  &jogStep1Button, &jogStep01Button, &jogStep001Button,
                                  &spindleSpeedCaption, &spindleToggleButton, &spindleIncreaseSpeedButton, &spindleDecreaseSpeedButton, &spindleDirButton, 
                                  &macroButton, &miscButton, &holdButton,
                                  &grblStateValue,
                                  NULL};

Window mainWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, mainWindowObjects);

void controllerInit(void)
{
    grblConnectInit();
    guiInit(&mainWindow);
}

void controllerTask(void)
{
    static char checkGrblCommand = 0;

    guiTask();

    if(grblGetState() != grblState)
    {
        grblState = grblGetState();
        switch(grblState)
        {
            case 0: grblStateValue.setText("Idle"); break;
            case 1: 
                grblStateValue.setText("Alarm"); 
                holdButton.setText("Clr");
                break;
            case 8: grblStateValue.setText("Run"); break;
            case 16: grblStateValue.setText("Hold"); break;
            case 32: grblStateValue.setText("Jog"); break;
            default: grblStateValue.setText("Unk"); break;
        }
        grblStateValue.setNeedToRedraw(true);
    }

    // if(guiGetCurrentWindow() == &miscWindow)
    // {
    //     uint32_t newAngle;
    //     if(grblModbusReadAbsPos(&newAngle))
    //     {
    //         if(currentAngle != newAngle)
    //         {
    //             currentAngle = newAngle;
    //             currentAngleNUV.setNeedToRedraw(true);
    //         }
    //         printf("%d %d\n", newAngle, grblYMPos);
    //     }
    // }

    if(grblCommandInProgress)
    {
        UserEvent event = etUserNone;

        if((grblState == 1) || grblGetLastError()) event = etGrblError;
        else if(grblExecuteCommand(&checkGrblCommand)) event = etGrblOk;

        if(event != etUserNone)
        {
            grblCommandInProgress = false;
            while(commandsStackSize)
            {
                commandsStack[--commandsStackSize](event, NULL);
                if(grblCommandInProgress) break;
            }
        }
    }

    grblConnectTask();

    if(!jogCurrentAxis || !jogCurrentDivider)
    {
        handEncoderValue = 0;
    }
    else if(handEncoderValue)
    {
       // grblExecuteCommand("G91G0%s%.3f\n", jogCurrentAxis, (float)handEncoderValue / jogCurrentDivider);
        if(grblExecuteCommand(NULL, "$J=G91%s%.3fF500\n", jogCurrentAxis, (float)handEncoderValue / jogCurrentDivider))
        {
            handEncoderValue = 0;
        }
    }


}
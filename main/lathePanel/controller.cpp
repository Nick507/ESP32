#include "controller.h"
#include "gui.h"
#include "grblConnect.h"
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <cstdarg>
#include <cassert>

// espefuse.py -p COM16 set_flash_voltage 3.3V

extern Window mainWindow;
extern Window threadWindow;

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
enum SpindleState {ssRunning, ssRun, ssDecelerating, ssStopping, ssStopped};
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

extern Button threadWindowButton;

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

//=====================================================================================
//
//                             Thread
//
//=====================================================================================
#define DEG2RAD(x) ((x) * M_PI / 180.0)

float threadPitch = 1, threadLength = -10, threadFirstCut = 0.1, threadDepth = 0.61, threadAngle = 30, threadRegression = 1.1;
uint32_t threadFeed = 1000, threadSpringPasses = 2;

void threadPitchValueCB(UserEvent event, RectangleObject * obj);

NamedUnitsValue threadPitchNUV       (10, 10 + 38 * 0, nvFloat, &threadPitch, "%5.2f", GUI_BUTTON_DEFAULT_FONT, "Pitch", "mm", &threadPitchValueCB);
NamedUnitsValue threadLengthNUV      (10, 10 + 38 * 1, nvFloat, &threadLength, "%7.2f", GUI_BUTTON_DEFAULT_FONT, "Length", "mm", NULL);
NamedUnitsValue threadFirstCutNUV    (10, 10 + 38 * 2, nvFloat, &threadFirstCut, "%5.2f", GUI_BUTTON_DEFAULT_FONT, "First cut", "mm", NULL);
Caption threadFirstCutHint           (280, 17 + 38 * 2, 0, 0, "(neg for internal)");
NamedUnitsValue threadDepthNUV       (10, 10 + 38 * 3, nvFloat, &threadDepth, "%5.2f", GUI_BUTTON_DEFAULT_FONT, "Depth", "mm", NULL);
NamedUnitsValue threadAngleNUV       (10, 10 + 38 * 4, nvFloat, &threadAngle, "%5.1f", GUI_BUTTON_DEFAULT_FONT, "Angle", "deg", NULL);
NamedUnitsValue threadRegressionNUV  (10, 10 + 38 * 5, nvFloat, &threadRegression, "%5.2f", GUI_BUTTON_DEFAULT_FONT, "Regression", NULL, NULL);
NamedUnitsValue threadFeedNUV        (10, 10 + 38 * 6, nvInt32, &threadFeed, "%4d", GUI_BUTTON_DEFAULT_FONT, "Feed", "mm/min", NULL);
NamedUnitsValue threadSpringPassesNUV(10, 10 + 38 * 7, nvInt32, &threadSpringPasses, "%2d", GUI_BUTTON_DEFAULT_FONT, "Spring passes", NULL, NULL);

void threadPitchValueCB(UserEvent event, RectangleObject * obj)
{
    threadDepth = 0.61 * threadPitch; // 0.61 = 0.866 * 17/24   0.866 = cos(60/2)
    threadDepthNUV.getValue().setNeedToRedraw(true);
}

void threadWindowButtonCB(UserEvent event, RectangleObject * obj)
{
    guiChangeWindow(&threadWindow);
}

void threadBackButtonCB(UserEvent event, RectangleObject * obj)
{
    guiChangeWindow(&mainWindow);
}

void threadRunButtonCB(UserEvent event, RectangleObject * obj); // forward declaration

Button threadBackButton(370 , 260, 100, 50, "Back", GUI_BUTTON_DEFAULT_FONT, &threadBackButtonCB);
Button threadRunButton(370 , 10, 100, 50, "Run", GUI_BUTTON_DEFAULT_FONT, &threadRunButtonCB);

RectangleObject * threadWindowObjects[] = {&threadPitchNUV, &threadLengthNUV, &threadFirstCutNUV, &threadDepthNUV,
                                    &threadAngleNUV, &threadRegressionNUV, &threadFeedNUV, &threadBackButton, &threadRunButton,
                                    &threadSpringPassesNUV, &threadFirstCutHint, NULL};

Window threadWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, threadWindowObjects);

void threadRunButtonCB(UserEvent event, RectangleObject * obj)
{
    static uint8_t state = 0;
    static uint16_t pass, springPass;
    static float cutDirection, targetX, targetY, targetZ, startX, startZ;
   // static uint16_t r5, r6, r21;

    if(event == etGrblError) // handle error in any state
    {
        //spindleCommandLock(NULL);
        state = 0;
        return;
    }

    switch(state)
    {
        case 0: // idle
            {
                if(obj != &threadRunButton) return;

                // if(!grblModbusWriteReg(5, 250, &r5)   ||
                //    !grblModbusWriteReg(6, 10, &r6)    ||
                //    !grblModbusWriteReg(21, 120, &r21))
                // {
                //     printf("Thread command failed to set modbus registers\n");
                //     return;
                // }

                //spindleCommandLock();
                guiChangeWindow(&mainWindow);

                cutDirection = threadFirstCut > 0 ? -1.0f : 1.0f;
                targetX = grblXPosition;
                targetY = grblYPosition;
                targetZ = grblZPosition;
                startX = grblXPosition;
                startZ = grblZPosition;
                pass = 1;
                springPass = 0;
                state = 1;
            }
            __attribute__ ((fallthrough));

        case 1: // start loop
            {
                if(springPass > threadSpringPasses)
                {
                    //spindleCommandLock(NULL);
                    state = 0;

                    // if(!grblModbusWriteReg(5, r5, NULL) ||
                    //    !grblModbusWriteReg(6, r6, NULL) ||
                    //    !grblModbusWriteReg(21, r21, NULL))
                    // {
                    //     printf("Thread command failed to restore modbus registers\n");
                    // }
                    break;
                }

                float doc = fabs(threadFirstCut) * powf((float)pass++, 1.0f / threadRegression);
                if(doc > threadDepth)
                {
                    doc = threadDepth;
                    springPass++;
                }

                targetX = startX + doc * cutDirection;
                targetZ = startZ - doc * tanf(DEG2RAD(threadAngle));
                grblExecuteCommand(threadRunButtonCB, "G0X%.3fZ%.3f\n", targetX, targetZ);
                state = 2;
            }
            break;
        
        case 2: // main part
            targetZ += threadLength;
	        targetY += fabs(threadLength / threadPitch);
            grblExecuteCommand(threadRunButtonCB, "G1Y%.3fZ%.3fF%d\n", targetY, targetZ, threadFeed);
            state = 3;
            break;
        
        case 3: // make exit taper
            targetY = (int)targetY + 5/*fixed amount of rotations*/;
            grblExecuteCommand(threadRunButtonCB, "G1Y%.3fF%d\n", targetY, threadFeed);
            state = 4;
            break;
        
        case 4: // retract
            targetX = startX + -.5/*fixed retract*/ * cutDirection;
            grblExecuteCommand(threadRunButtonCB, "G0X%.3f\n", targetX);
            state = 5;
            break;
        
        case 5: // back to start
            targetZ = startZ;
            grblExecuteCommand(threadRunButtonCB, "G0Z%.3f\n", targetZ);
            state = 1;
            break;
    }
}

// void threadRunButtonCB(UserEvent event, RectangleObject * obj)
// {
//     grblExecuteCommand("G18\nG76 P%.2f Z%.2f I-.01 J%.2f K%.2f Q%.2f R%.2f F%d\n", 
//             threadPitch, threadLength, threadFirstCut, threadDepth, threadAngle, threadRegression, threadFeed);
//     guiChangeWindow(&mainWindow);
// }

//=====================================================================================
//
//                             Turning
//
//=====================================================================================

extern Window turningWindow;

float turningDeltaDiameter = -1, turningRoughPassDepth = 0.2, turningFinishPassDepth = 0.1, turningLength = -10, turningRetract = 0.1;
uint32_t turningRoughFeed = 200, turningFinishFeed = 80, turningSpindleSpeed = 1000;

NamedUnitsValue turningDeltaDiameterNUV   (10, 10 + 38 * 0, nvFloat, &turningDeltaDiameter, "%6.2f", GUI_BUTTON_DEFAULT_FONT, "Delta D", "mm", NULL);
NamedUnitsValue turningLengthNUV          (10, 10 + 38 * 1, nvFloat, &turningLength, "%6.2f", GUI_BUTTON_DEFAULT_FONT, "Length", "mm", NULL);
NamedUnitsValue turningRoughCutDepthNUV   (10, 10 + 38 * 2, nvFloat, &turningRoughPassDepth, "%5.2f", GUI_BUTTON_DEFAULT_FONT, "Rough cut (D)", "mm", NULL);
NamedUnitsValue turningRoughFeedNUV       (10, 10 + 38 * 3, nvInt32, &turningRoughFeed, "%4d", GUI_BUTTON_DEFAULT_FONT, "Rough feed", "mm/min", NULL);
NamedUnitsValue turningFinishCutDepthNUV  (10, 10 + 38 * 4, nvFloat, &turningFinishPassDepth, "%5.2f", GUI_BUTTON_DEFAULT_FONT, "Finish cut (D)", "mm", NULL);
NamedUnitsValue turningFinishFeedNUV      (10, 10 + 38 * 5, nvInt32, &turningFinishFeed, "%4d", GUI_BUTTON_DEFAULT_FONT, "Finish feed", "mm/min", NULL);
NamedUnitsValue turningRetractNUV         (10, 10 + 38 * 6, nvFloat, &turningRetract, "%2.2f", GUI_BUTTON_DEFAULT_FONT, "Retract", "mm", NULL);
NamedUnitsValue turningSpindleSpeedNUV    (10, 10 + 38 * 7, nvInt32, &turningSpindleSpeed, "%4d", GUI_BUTTON_DEFAULT_FONT, "Spindle", "RPM", NULL);


void turningBackButtonCB(UserEvent event, RectangleObject * obj)
{
    guiChangeWindow(&mainWindow);
}

void turningRunButtonCB(UserEvent event, RectangleObject * obj); // forward declaration

void turningRunButtonCB(UserEvent event, RectangleObject * obj);

Button turningRunButton(370, 10, 100, 50, "Run", GUI_BUTTON_DEFAULT_FONT, &turningRunButtonCB);
Button turningPauseButton(370, 70, 100, 50, "Pause", GUI_BUTTON_DEFAULT_FONT, NULL);
Button turningStopButton(370, 130, 100, 50, "Stop", GUI_BUTTON_DEFAULT_FONT, NULL);
Button turningRetractButton(370, 190, 100, 50, "Retract", GUI_BUTTON_DEFAULT_FONT, NULL);
Button turningBackButton(370, 250, 100, 50, "Back", GUI_BUTTON_DEFAULT_FONT, &turningBackButtonCB);

void turningWindowButtonCB(UserEvent event, RectangleObject * obj)
{
    turningPauseButton.setEnabled(false);
    turningStopButton.setEnabled(false);
    turningRetractButton.setEnabled(false);
    guiChangeWindow(&turningWindow);
}


RectangleObject * turningWindowObjects[] = {&turningDeltaDiameterNUV, &turningRoughCutDepthNUV, &turningFinishCutDepthNUV, &turningRetractNUV, 
                                            &turningRoughFeedNUV, &turningFinishFeedNUV, &turningLengthNUV, &turningSpindleSpeedNUV,
                                            &turningRunButton, &turningPauseButton, &turningStopButton, &turningRetractButton, &turningBackButton, NULL};

Window turningWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, turningWindowObjects);

void turningRunButtonCB(UserEvent event, RectangleObject * obj)
{
    static uint8_t state = 0;
    static float cutDirection, targetX, startX, startZ, currentCutDepth;
    static int16_t pass = 0, feed = 0;

    if(event == etGrblError) // handle error in any state
    {
        //spindleCommandLock(NULL);
        state = 0;
        return;
    }

    switch(state)
    {
        case 0: // idle
            {
                if(obj != &turningRunButton) return;

                // check that we can execute, and push our callback to get control after spindle start
                if(!grblExecuteCommand(turningRunButtonCB, "")) return;
                //spindleCommandLock(turningRunButtonCB);
                guiChangeWindow(&mainWindow);

                cutDirection = turningDeltaDiameter < 0 ? -1.0f : 1.0f;
                targetX = grblXPosition;
                startX = grblXPosition;
                startZ = grblZPosition;
                currentCutDepth = 0;
                state = 1;
                pass = 1;
                feed = turningRoughFeed;

                spindleSpeed = turningSpindleSpeed;
                
                if(spindleState == ssStopped) spindleToggleButtonCB(etButtonPressed, NULL);
            }
            break;

        case 1: // start loop
            {
                float diff = fabs(turningDeltaDiameter) - currentCutDepth;

                if(diff == 0)
                {
                    state = 5;
                    grblExecuteCommand(turningRunButtonCB, "G0X%.3f\n", targetX);
                    break;
                }

                if(diff >= turningRoughPassDepth + turningFinishPassDepth)
                {
                    currentCutDepth = pass++ * turningRoughPassDepth;
                } 
                else if(diff <= turningFinishPassDepth + 0.001 /*tolerance*/)
                {
                    currentCutDepth = fabs(turningDeltaDiameter);
                    feed = turningFinishFeed;
                }
                else
                {
                    currentCutDepth = fabs(turningDeltaDiameter) - turningFinishPassDepth;
                }

                targetX = startX + currentCutDepth * cutDirection / 2; 
                grblExecuteCommand(turningRunButtonCB, "G0X%.3f\n", targetX);
                state = 2;
            }
            break;
        
        case 2: // main part
            grblExecuteCommand(turningRunButtonCB, "G1Z%.3fF%d\n", startZ + turningLength, feed);
            state = 3;
            break;
        
        case 3: // retract
            grblExecuteCommand(turningRunButtonCB, "G0X%.3f\n", targetX - turningRetract * cutDirection);
            state = 4;
            break;
        
        case 4: // back to start
            grblExecuteCommand(turningRunButtonCB, "G0Z%.3f\n", startZ);
            state = 1;
            break;

        case 5: // stop
            state = 0;
            if(spindleState == ssRun) spindleToggleButtonCB(etButtonPressed, NULL);
            break;
    }
}

//=====================================================================================
//
//                             Cone
//
//=====================================================================================

float coneAngle = 10;
NamedUnitsValue coneAngleNUV (10, 10 + 38 * 0, nvFloat, &coneAngle, "%6.2f", GUI_BUTTON_DEFAULT_FONT, "Cone angle", "deg", NULL);
Caption coneAngleHint        (10, 10 + 38 * 1, 0, 0, "(negative angle for internal)");

extern Window coneWindow;

void coneBackButtonCB(UserEvent event, RectangleObject * obj)
{
    guiChangeWindow(&mainWindow);
}

void coneRunButtonCB(UserEvent event, RectangleObject * obj); // forward declaration

void coneRunButtonCB(UserEvent event, RectangleObject * obj);

Button coneRunButton(370, 10, 100, 50, "Run", GUI_BUTTON_DEFAULT_FONT, &coneRunButtonCB);
Button conePauseButton(370, 70, 100, 50, "Pause", GUI_BUTTON_DEFAULT_FONT, NULL);
Button coneStopButton(370, 130, 100, 50, "Stop", GUI_BUTTON_DEFAULT_FONT, NULL);
Button coneRetractButton(370, 190, 100, 50, "Retract", GUI_BUTTON_DEFAULT_FONT, NULL);
Button coneBackButton(370, 250, 100, 50, "Back", GUI_BUTTON_DEFAULT_FONT, &coneBackButtonCB);

void coneWindowButtonCB(UserEvent event, RectangleObject * obj)
{
    conePauseButton.setEnabled(false);
    coneStopButton.setEnabled(false);
    coneRetractButton.setEnabled(false);
    guiChangeWindow(&coneWindow);
}


RectangleObject * coneWindowObjects[] = {&coneAngleHint, &coneAngleNUV, &turningRetractNUV, 
                                            &turningLengthNUV, &turningSpindleSpeedNUV,
                                            &coneRunButton, &conePauseButton, &coneStopButton, &coneRetractButton, &coneBackButton, NULL};

Window coneWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, coneWindowObjects);

void coneRunButtonCB(UserEvent event, RectangleObject * obj)
{
    static uint8_t state = 0;
    static float cutDirection, targetX, targetZ, startX, startZ, currentCutDepth;
    static int16_t pass = 0;

    if(event == etGrblError) // handle error in any state
    {
        //spindleCommandLock(NULL);
        state = 0;
        return;
    }

    switch(state)
    {
        case 0: // idle
            {
                if(obj != &coneRunButton) return;

                // check that we can execute, and push our callback to get control after spindle start
                if(!grblExecuteCommand(coneRunButtonCB, "")) return;
                //spindleCommandLock(turningRunButtonCB);
                guiChangeWindow(&mainWindow);

                cutDirection = coneAngle > 0 ? -1.0f : 1.0f;
                targetX = grblXPosition;
                targetZ = grblZPosition;
                startX = grblXPosition;
                startZ = grblZPosition;
                currentCutDepth = 0;
                turningDeltaDiameter = sin(DEG2RAD(fabs(coneAngle))) * fabs(turningLength);
                state = 1;
                pass = 1;

                spindleSpeed = turningSpindleSpeed;

                if(spindleState == ssStopped) spindleToggleButtonCB(etButtonPressed, NULL);
            }
            break;

        case 1: // start loop
            {
                if(fabs(turningDeltaDiameter) - currentCutDepth)
                {
                    //spindleCommandLock(NULL);
                    state = 0;
                    if(spindleState == ssRun) spindleToggleButtonCB(etButtonPressed, NULL);
                    break;
                }

                float diff = fabs(turningDeltaDiameter) - currentCutDepth;

                if(diff > 2 * turningRoughPassDepth)
                {
                    currentCutDepth = pass++ * turningRoughPassDepth;
                } 
                else if(diff < turningRoughPassDepth)
                {
                    currentCutDepth = fabs(turningDeltaDiameter);
                }
                else
                {
                    currentCutDepth += diff / 2;
                }

                targetX = startX + currentCutDepth * cutDirection / cos(DEG2RAD(coneAngle)) / 2; 
                grblExecuteCommand(coneRunButtonCB, "G0X%.3f\n", targetX);
                state = 2;
            }
            break;
        
        case 2: // main part
            if(turningLength < 0)
                targetZ -= currentCutDepth / sin(DEG2RAD(fabs(coneAngle)));
            else
                targetZ += currentCutDepth / sin(DEG2RAD(fabs(coneAngle)));
            targetX = startX;
            grblExecuteCommand(coneRunButtonCB, "G1X%.3fZ%.3fF%d\n", targetX, targetZ, turningRoughFeed);
            state = 3;
            break;
        
        case 3: // retract
            targetX = startX - turningRetract * cutDirection / cos(DEG2RAD(coneAngle));
            grblExecuteCommand(coneRunButtonCB, "G0X%.3f\n", targetX);
            state = 4;
            break;
        
        case 4: // back to start
            targetZ = startZ;
            targetX = startX + (currentCutDepth - turningRetract) * cutDirection / cos(DEG2RAD(coneAngle)) / 2; 
            grblExecuteCommand(coneRunButtonCB, "G0X%.3fZ%.3f\n", targetX, targetZ);
            state = 1;
            break;
    }
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
//                      FILE SCREEN      
//
//=====================================================================================

char fileListBuf[1024];
MultilineText fileList(10, 10, 350, 300, fileListBuf);

void fileBackButtonCB(UserEvent event, RectangleObject * obj)
{
    guiChangeWindow(&mainWindow);
}

void fileRunButtonCB(UserEvent event, RectangleObject * obj)
{
    const char * fileName = fileList.getLineText(fileList.getSelectedLineNumber());
    if(fileName)
    {
        grblExecuteCommand(NULL, "$F=%s", fileName);
        guiChangeWindow(&mainWindow);
    }
}

Button fileBackButton(370, 260, 100, 50, "Back", GUI_BUTTON_DEFAULT_FONT, &fileBackButtonCB);
Button fileRunButton(370, 10, 100, 50, "Run", GUI_BUTTON_DEFAULT_FONT, &fileRunButtonCB);

RectangleObject * fileWindowObjects[] = {&fileList, &fileRunButton, &fileBackButton, NULL};

Window fileWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, fileWindowObjects);

void fileButtonCB(UserEvent event, RectangleObject * obj)
{
    //grblExecuteCommand("$F+");
    grblListFiles(fileListBuf, sizeof(fileListBuf));
    //printf(buf);
    guiChangeWindow(&fileWindow);
}

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

void miscBackButtonCB(UserEvent event, RectangleObject * obj)
{
    guiChangeWindow(&mainWindow);
}

char captionIPAddr[16] = {"0.0.0.0"};

Button enaButton(10, 10, 90, 50, "Enable", GUI_BUTTON_DEFAULT_FONT, enaButtonCB);
Button disButton(120, 10, 90, 50, "Disable", GUI_BUTTON_DEFAULT_FONT, disButtonCB);
Button resetButton(230, 10, 90, 50, "Reset", GUI_BUTTON_DEFAULT_FONT, resetButtonCB);
Caption ipAddr(10, 270, 150, 50, captionIPAddr, GUI_BUTTON_DEFAULT_FONT);
Button miscBackButton(370, 260, 100, 50, "Back", GUI_BUTTON_DEFAULT_FONT, &miscBackButtonCB);

RectangleObject * miscWindowObjects[] = {&enaButton, &disButton, &resetButton, &miscBackButton, &ipAddr, NULL};

Window miscWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, miscWindowObjects);

void miscButtonCB(UserEvent event, RectangleObject * obj)
{
    strcpy(captionIPAddr, grblGetIP());
    guiChangeWindow(&miscWindow);
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

Button threadWindowButton(10, 140, 100, 50, "Thread", GUI_BUTTON_DEFAULT_FONT, &threadWindowButtonCB);
Button miscButton(120, 140, 70, 50, "Misc", GUI_BUTTON_DEFAULT_FONT, miscButtonCB);
Button fileButton(200, 140, 70, 50, "File", GUI_BUTTON_DEFAULT_FONT, fileButtonCB);
Button turningWindowButton(10, 200, 100, 50, "Turning", GUI_BUTTON_DEFAULT_FONT, &turningWindowButtonCB);
Button coneWindowButton(120, 200, 70, 50, "Cone", GUI_BUTTON_DEFAULT_FONT, &coneWindowButtonCB);

RectangleObject * mainWindowObjects[] = {&xPositionCaption, &yPositionCaption, &zPositionCaption, 
                                  &zeroXPositionButton, &zeroYPositionButton, &zeroZPositionButton, 
                                  &jogXAxisButton, &jogYAxisButton, &jogZAxisButton, 
                                  &jogStep1Button, &jogStep01Button, &jogStep001Button,
                                  &threadWindowButton, 
                                  &spindleSpeedCaption, &spindleToggleButton, &spindleIncreaseSpeedButton, &spindleDecreaseSpeedButton, &spindleDirButton, 
                                  &miscButton, &holdButton, &fileButton, &turningWindowButton, &coneWindowButton,
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

    if(grblCommandInProgress)
    {
        UserEvent event = etNone;

        if((grblState == 1) || grblGetLastError()) event = etGrblError;
        else if(grblExecuteCommand(&checkGrblCommand)) event = etGrblOk;

        if(event != etNone)
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
#include "thread.h"
#include "controller.h"
#include "macro.h"
#include <math.h>


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
    guiChangeWindow(&macroWindow);
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
                targetX = grblGetXPosition();
                targetY = grblGetYPosition();
                targetZ = grblGetZPosition();
                startX = grblGetXPosition();
                startZ = grblGetZPosition();
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
                grblExecuteCommandBuffered(threadRunButtonCB, "G0X%.3fZ%.3f\n", targetX, targetZ);
                state = 2;
            }
            break;
        
        case 2: // main part
            targetZ += threadLength;
	        targetY += fabs(threadLength / threadPitch);
            grblExecuteCommandBuffered(threadRunButtonCB, "G1Y%.3fZ%.3fF%d\n", targetY, targetZ, threadFeed);
            state = 3;
            break;
        
        case 3: // make exit taper
            targetY = (int)targetY + 5/*fixed amount of rotations*/;
            grblExecuteCommandBuffered(threadRunButtonCB, "G1Y%.3fF%d\n", targetY, threadFeed);
            state = 4;
            break;
        
        case 4: // retract
            targetX = startX + -.5/*fixed retract*/ * cutDirection;
            grblExecuteCommandBuffered(threadRunButtonCB, "G0X%.3f\n", targetX);
            state = 5;
            break;
        
        case 5: // back to start
            targetZ = startZ;
            grblExecuteCommandBuffered(threadRunButtonCB, "G0Z%.3f\n", targetZ);
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

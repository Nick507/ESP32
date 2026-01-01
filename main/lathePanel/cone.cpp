#include "cone.h"
#include "controller.h"
#include "macro.h"
#include <math.h>

//=====================================================================================
//
//                             Cone
//
//=====================================================================================

#define DEG2RAD(x) ((x) * M_PI / 180.0)

// Import turning parameters
extern float turningRoughPassDepth, turningFinishPassDepth, turningLength, turningRetract, turningDeltaDiameter;
extern uint32_t turningRoughFeed, turningFinishFeed, turningSpindleSpeed;
extern NamedUnitsValue turningLengthNUV, turningRoughCutDepthNUV, turningFinishCutDepthNUV;
extern NamedUnitsValue turningRetractNUV, turningRoughFeedNUV, turningFinishFeedNUV, turningSpindleSpeedNUV;

float coneAngle = 10;
NamedUnitsValue coneAngleNUV (10, 10 + 38 * 0, nvFloat, &coneAngle, "%6.2f", GUI_BUTTON_DEFAULT_FONT, "Cone angle", "deg", NULL);

void coneBackButtonCB(UserEvent event, RectangleObject * obj)
{
    guiChangeWindow(&macroWindow);
}

void coneRunButtonCB(UserEvent event, RectangleObject * obj); // forward declaration

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


RectangleObject * coneWindowObjects[] = {&coneAngleNUV, &turningLengthNUV, &turningRoughCutDepthNUV, &turningRoughFeedNUV, 
                                         &turningFinishCutDepthNUV, &turningFinishFeedNUV,
                                         &turningRetractNUV, &turningSpindleSpeedNUV,
                                         &coneRunButton, &conePauseButton, &coneStopButton, &coneRetractButton, &coneBackButton, NULL};

Window coneWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, coneWindowObjects);

void coneRunButtonCB(UserEvent event, RectangleObject * obj)
{
    static uint8_t state = 0;
    static float cutDirection, targetX, targetZ, startX, startZ, currentCutDepth;
    static int16_t pass = 0, feed;

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
                targetX = grblGetXPosition();
                targetZ = grblGetZPosition();
                startX = grblGetXPosition();
                startZ = grblGetZPosition();
                currentCutDepth = 0;
                turningDeltaDiameter = sin(DEG2RAD(fabs(coneAngle))) * fabs(turningLength);
                state = 1;
                pass = 1;
                feed = turningRoughFeed;

                setSpindleSpeed(turningSpindleSpeed);

                if(getSpindleState() == ssStopped) spindleToggle();
            }
            break;

        case 1: // start loop
            {
                float diff = fabs(turningDeltaDiameter) - currentCutDepth;

                if(diff == 0)
                {
                    state = 0;
                    if(getSpindleState() == ssRun) spindleToggle();
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
            grblExecuteCommand(coneRunButtonCB, "G1X%.3fZ%.3fF%d\n", targetX, targetZ, feed);
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


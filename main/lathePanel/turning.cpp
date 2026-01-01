#include "turning.h"
#include "controller.h"
#include "macro.h"
#include <math.h>

//=====================================================================================
//
//                             Turning
//
//=====================================================================================

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
    guiChangeWindow(&macroWindow);
}

void turningRunButtonCB(UserEvent event, RectangleObject * obj); // forward declaration

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
                targetX = grblGetXPosition();
                startX = grblGetXPosition();
                startZ = grblGetZPosition();
                currentCutDepth = 0;
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
            if(getSpindleState() == ssRun) spindleToggle();
            break;
    }
}


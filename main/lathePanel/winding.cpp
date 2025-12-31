#include "winding.h"
#include "controller.h"
#include <math.h>

//=====================================================================================
//
//                             Winding
//
//=====================================================================================

uint32_t windingTurns = 15;
float windingWidth = 4.0, windingStep = 1.0;
uint32_t windingFeed = 50;
int32_t windingDirection = 1; // 1 for left to right, -1 for right to left

NamedUnitsValue windingTurnsNUV     (10, 10 + 38 * 0, nvInt32, &windingTurns, "%6d", GUI_BUTTON_DEFAULT_FONT, "Turns", NULL, NULL);
NamedUnitsValue windingWidthNUV     (10, 10 + 38 * 1, nvFloat, &windingWidth, "%5.2f", GUI_BUTTON_DEFAULT_FONT, "Width", "mm", NULL);
NamedUnitsValue windingStepNUV      (10, 10 + 38 * 2, nvFloat, &windingStep, "%5.2f", GUI_BUTTON_DEFAULT_FONT, "Step", "mm", NULL);
Caption windingStepHint             (280, 17 + 38 * 2, 0, 0, "(wire+gap)");
NamedUnitsValue windingFeedNUV      (10, 10 + 38 * 3, nvInt32, &windingFeed, "%4d", GUI_BUTTON_DEFAULT_FONT, "Feed", "mm/min", NULL);

void windingDirButtonCB(UserEvent event, RectangleObject * obj);

Caption windingDirectionCaption     (10, 17 + 38 * 4, 0, 0, "Direction");
Button windingDirButton             (120, 10 + 38 * 4, 80, 36, "=>", GUI_BUTTON_DEFAULT_FONT, &windingDirButtonCB);

void windingDirButtonCB(UserEvent event, RectangleObject * obj)
{
    windingDirection = -windingDirection;
    windingDirButton.setText(windingDirection == 1 ? "=>" : "<=");
}

void windingBackButtonCB(UserEvent event, RectangleObject * obj)
{
    guiChangeWindow(&mainWindow);
}

void windingRunButtonCB(UserEvent event, RectangleObject * obj); // forward declaration

Button windingBackButton(370, 260, 100, 50, "Back", GUI_BUTTON_DEFAULT_FONT, &windingBackButtonCB);
Button windingRunButton(370, 10, 100, 50, "Run", GUI_BUTTON_DEFAULT_FONT, &windingRunButtonCB);

RectangleObject * windingWindowObjects[] = {&windingTurnsNUV, &windingWidthNUV, &windingStepNUV, &windingStepHint,
                                            &windingFeedNUV, &windingDirectionCaption, &windingDirButton,
                                            &windingBackButton, &windingRunButton, NULL};

Window windingWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, windingWindowObjects);

void windingWindowButtonCB(UserEvent event, RectangleObject * obj)
{
    guiChangeWindow(&windingWindow);
}

void windingRunButtonCB(UserEvent event, RectangleObject * obj)
{
    static uint8_t state = 0;
    static uint32_t currentTurn;
    static int32_t turnsPerPass;
    static bool oddLayer;
    static int32_t turnsInCurrentPass;
    static float targetY, targetZ;

    if(event == etGrblError) // handle error in any state
    {
        state = 0;
        return;
    }

    switch(state)
    {
        case 0: // idle
            {
                if(obj != &windingRunButton) return;

                guiChangeWindow(&mainWindow);

                // Initialize winding - set feedrate, absolute positioning, reset position
                grblExecuteCommand(windingRunButtonCB, "F%d\n", windingFeed);
                state = 1;
            }
            break;

        case 1: // set absolute positioning
            grblExecuteCommand(windingRunButtonCB, "G90\n");
            state = 2;
            break;

        case 2: // reset position
            grblExecuteCommand(windingRunButtonCB, "G92 X0 Y0 Z0\n");
            
            // Initialize winding parameters
            turnsPerPass = (int32_t)(windingWidth / windingStep) - 1;
            oddLayer = false;
            turnsInCurrentPass = 1;
            currentTurn = 0;
            targetY = 0;
            targetZ = 0;
            state = 3;
            break;

        case 3: // winding loop
            {
                if(currentTurn >= windingTurns)
                {
                    state = 0; // done
                    break;
                }

                // Calculate Z position based on layer
                if(oddLayer)
                {
                    targetZ = (turnsPerPass - turnsInCurrentPass) * windingStep - windingStep / 2.0f;
                }
                else
                {
                    targetZ = turnsInCurrentPass * windingStep;
                }

                // Update layer tracking
                if(turnsInCurrentPass >= turnsPerPass - (oddLayer ? 1 : 0))
                {
                    oddLayer = !oddLayer;
                    turnsInCurrentPass = 0;
                }
                else
                {
                    turnsInCurrentPass++;
                }

                // Calculate Y position
                targetY = -(float)(currentTurn + 1);
                
                // Apply direction to Z
                float zPos = targetZ * windingDirection;
                
                // Send G-code command
                grblExecuteCommand(windingRunButtonCB, "G1 Y%.3f Z%.3f\n", targetY, zPos);
                
                currentTurn++;
                // Stay in state 3 to continue loop
            }
            break;
    }
}
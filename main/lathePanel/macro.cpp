#include "macro.h"
#include "controller.h"
#include "thread.h"
#include "turning.h"
#include "cone.h"
#include "winding.h"
#include "file.h"

//=====================================================================================
//
//                             MACRO MENU
//
//=====================================================================================

void macroBackButtonCB(UserEvent event, RectangleObject * obj)
{
    guiChangeWindow(&mainWindow);
}

Button macroThreadButton(10, 10, 100, 50, "Thread", GUI_BUTTON_DEFAULT_FONT, &threadWindowButtonCB);
Button macroTurningButton(120, 10, 100, 50, "Turning", GUI_BUTTON_DEFAULT_FONT, &turningWindowButtonCB);
Button macroConeButton(230, 10, 70, 50, "Cone", GUI_BUTTON_DEFAULT_FONT, &coneWindowButtonCB);
Button macroWindingButton(310, 10, 70, 50, "Wind", GUI_BUTTON_DEFAULT_FONT, &windingWindowButtonCB);
Button macroFileButton(390, 10, 70, 50, "File", GUI_BUTTON_DEFAULT_FONT, &fileButtonCB);
Button macroBackButton(370, 260, 100, 50, "Back", GUI_BUTTON_DEFAULT_FONT, &macroBackButtonCB);

RectangleObject * macroWindowObjects[] = {&macroThreadButton, &macroTurningButton, &macroConeButton, 
                                          &macroWindingButton, &macroFileButton, &macroBackButton, NULL};

Window macroWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, macroWindowObjects);

void macroWindowButtonCB(UserEvent event, RectangleObject * obj)
{
    guiChangeWindow(&macroWindow);
}


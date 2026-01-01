#include "grblConnect.h"
#include "file.h"
#include "controller.h"
#include "macro.h"

//=====================================================================================
//
//                      FILE SCREEN      
//
//=====================================================================================

char fileListBuf[1024];
MultilineText fileList(10, 10, 350, 300, fileListBuf);

void fileBackButtonCB(UserEvent event, RectangleObject * obj)
{
    guiChangeWindow(&macroWindow);
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


#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void controllerInit(void);
void controllerTask(void);

#ifdef __cplusplus
}
#endif

#include "gui.h"

bool grblExecuteCommand(ObjectCallbackPtr cb, const char* format, ...);
float grblGetXPosition(void);
float grblGetYPosition(void);
float grblGetZPosition(void);

extern Window mainWindow;
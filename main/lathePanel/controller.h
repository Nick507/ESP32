#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void controllerInit(void);
void controllerTask(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include "gui.h"

bool grblExecuteCommand(ObjectCallbackPtr cb, const char* format, ...);
bool grblExecuteCommandBuffered(ObjectCallbackPtr cb, const char* format, ...);
float grblGetXPosition(void);
float grblGetYPosition(void);
float grblGetZPosition(void);

// Spindle control
enum SpindleState {ssRunning, ssRun, ssDecelerating, ssStopping, ssStopped};
void setSpindleSpeed(uint32_t speed);
SpindleState getSpindleState(void);
void spindleToggle(void);

extern Window mainWindow;

#endif // __cplusplus
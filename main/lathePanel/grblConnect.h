#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern float grblXPosition;
extern float grblYPosition;
extern float grblZPosition;
extern volatile int32_t handEncoderValue;

void grblConnectInit();
bool grblExecuteCommand(char* command);
void grblConnectTask();
void setMotorState(uint32_t level);
void grblEnqueueRealtimeCommand(char c);
uint16_t grblGetState();
bool grblModbusWriteReg(uint16_t reg, uint16_t value, uint16_t * prevValue);
bool grblModbusReadReg(uint16_t reg, uint16_t * value);
void grblListFiles(char * buf, uint16_t len);
void grblRestart();
const char * grblGetIP();
int grblGetLastError();

#ifdef __cplusplus
}
#endif
#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "grbl/core_handlers.h"
#include "grbl/state_machine.h"
#include "grbl/modbus.h"
#include "grbl/hal.h"
#include "grbl/vfs.h"
#include "wifi.h"
#include "lathePanel/grblConnect.h"
#include "boards/my_machine_map.h"
#include "esp_ipc.h"

// ================ exposed variables =============
float grblXPosition;
float grblYPosition;
float grblZPosition;
volatile int32_t handEncoderValue = 0;

static void IRAM_ATTR handEncoderInterruptHandler(void *args)
{
    static unsigned char state = 0;
    const unsigned char sm[] = {0x00, 0x10, 0x40, 0x03, 
                                0x00, 0x10, 0x13, 0x20, 
                                0x23, 0x10, 0x30, 0x20, 
                                0x01, 0x33, 0x30, 0x20,
                                0x00, 0x43, 0x40, 0x50,
                                0x53, 0x60, 0x40, 0x50,
                                0x02, 0x60, 0x63, 0x50};
    
    unsigned char pinStates = (gpio_get_level(HAND_ENCODER_CH_A_PIN) ? 1 : 0) | (gpio_get_level(HAND_ENCODER_CH_B_PIN) ? 2 : 0);
    unsigned char r = sm[(state << 2) + pinStates];

    state = r >> 4;
    switch(r & 0x0F)
    {
        case 1: handEncoderValue++; break;
        case 2: handEncoderValue--; break;
        case 3: ets_printf("ENCODER ERROR\n"); break;
    }
}

void initHandEncoder()
{
    gpio_config_t gpio_conf =
    {
        .pin_bit_mask = (1ULL << HAND_ENCODER_CH_A_PIN) | (1ULL << HAND_ENCODER_CH_B_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_ANYEDGE
    };

    gpio_config(&gpio_conf);

    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    gpio_isr_handler_add(HAND_ENCODER_CH_A_PIN, handEncoderInterruptHandler, NULL);
    gpio_isr_handler_add(HAND_ENCODER_CH_B_PIN, handEncoderInterruptHandler, NULL);
}

void grblConnectInit()
{
    initHandEncoder();
}

// stream_write_ptr prevStreamWritePtr = NULL;
// static char writeBuf[1024];
// static uint16_t writeBufPos = 0;

// static void grblStreamWriteProxy(const char *s)
// {
//     while(*s)
//     {
//         if(writeBufPos >= sizeof(writeBuf) - 1) break;
//         writeBuf[writeBufPos++] = *s++;
//     }
// }

bool grblExecuteCommand(char* command)
{
    // writeBufPos = 0;
    // prevStreamWritePtr = hal.stream.write;
    // hal.stream.write = &grblStreamWriteProxy;

    bool res;

    if(command[0] == '$' && command[1] != 'J') // jog command to be executed by enqueue_gcode
    {
        res = (system_execute_line(command) == Status_OK);
    }
    else
    {
        res = grbl.enqueue_gcode(command); // = protocol_enqueue_gcode;
    }

    if(command[0]) // do not spam with test empty command
    {
        printf(">(%s) %s", res ? "OK" : "FAILED", command);
        if(!strchr(command, '\n')) printf("\n"); // line feed for commands without it to flush to console
    }

    // hal.stream.write = prevStreamWritePtr;
    // writeBuf[writeBufPos] = 0;
    // printf("<%s", writeBuf);
    return res;
}

void getPositions()
{
    int32_t curpos[N_AXIS];
    memcpy(curpos, sys.position, sizeof(sys.position));
    float mpos[N_AXIS];
    system_convert_array_steps_to_mpos(mpos, curpos);
    grblXPosition = mpos[X_AXIS];
    grblYPosition = mpos[Y_AXIS];
    grblZPosition = mpos[Z_AXIS];

    // apply G92 offset
    grblXPosition -= gc_state.g92_coord_offset[X_AXIS];
    grblYPosition -= gc_state.g92_coord_offset[Y_AXIS];
    grblZPosition -= gc_state.g92_coord_offset[Z_AXIS];
}

void grblConnectTask()
{
    getPositions();
}

void setMotorState(uint32_t level)
{
    gpio_set_level(STEPPERS_ENABLE_PIN, level);
}

void grblEnqueueRealtimeCommand(char c)
{
    stream_enqueue_realtime_command(c);
}

uint16_t grblGetState()
{
    return state_get();
}

// ========================================= MODBUS =================================

static void modbusRecvCB (modbus_message_t *msg);
static void modbusErrCB (uint8_t code, void *context);

static const modbus_callbacks_t modbusCallbacks = 
{
    .on_rx_packet = modbusRecvCB,
    .on_rx_exception = modbusErrCB
};

static void modbusRecvCB(modbus_message_t *msg)
{
    //printf("modbusRecvCB:\n");
    //for(int i = 0; i < 7; i++) printf("%02X ", msg->adu[i]);
    //printf("\n");

    if(msg->context)
    {
        if(!(msg->adu[1] & 0x80)) 
        {
            *((uint16_t*)msg->context) = (msg->adu[3] << 8) | msg->adu[4];
        }
    }
}

static void modbusErrCB(uint8_t code, void *context)
{
    printf("modbusErrCB\n");
}

bool grblModbusWriteReg(uint16_t reg, uint16_t value, uint16_t * prevValue)
{
    if(prevValue)
    {
        if(!grblModbusReadReg(reg, prevValue)) return false;
    }

    modbus_message_t msg = 
    {
        .context = (void *)NULL,
        .crc_check = true,
        .adu[0] = 1, // TODO: read from config!!!
        .adu[1] = ModBus_WriteRegister,
        .adu[2] = reg >> 8,
        .adu[3] = reg & 0xFF,
        .adu[4] = value >> 8,
        .adu[5] = value & 0xFF,
        .tx_length = 8,
        .rx_length = 8
    };

    for(int i = 0; i < 25; i++)
    {
        if(modbus_send(&msg, &modbusCallbacks, true /*block*/)) return true;
    }

    return false;
}

bool grblModbusReadReg(uint16_t reg, uint16_t * value)
{
    modbus_message_t msg = 
    {
        .context = (void *)value,
        .crc_check = true,
        .adu[0] = 1, // TODO: read from config!!!
        .adu[1] = ModBus_ReadHoldingRegisters,
        .adu[2] = reg >> 8,
        .adu[3] = reg & 0xFF,
        .adu[4] = 0x00,
        .adu[5] = 0x01,
        .tx_length = 8,
        .rx_length = 7
    };

    for(int i = 0; i < 25; i++)
    {
        if(modbus_send(&msg, &modbusCallbacks, true /*block*/)) return true;
    }

    return false;
}

// ==================================== FILES ==========================
void grblListFiles(char * buf, uint16_t len)
{
    vfs_dir_t *dir;
    vfs_dirent_t *dirent;

    int pos = 0;
    buf[0] = 0;
    buf[1] = 0;

    if((dir = vfs_opendir("/")) == NULL) return;

    while(true) 
    {
        if((dirent = vfs_readdir(dir)) == NULL || dirent->name[0] == '\0') break;
        if(dirent->st_mode.directory) continue;

        size_t nameLen = strlen(dirent->name);
        if(pos + nameLen + 2 >= len) break;
        strcpy(buf + pos, dirent->name); //dirent->size
        pos = pos + nameLen + 1;
    }
    buf[pos] = 0;

    vfs_closedir(dir);
}

void grblRestart()
{
    //grblEnqueueRealtimeCommand(0x18);
    esp_restart();
}

const char * grblGetIP()
{
    return wifi_get_ipaddr();
}

int grblGetLastError()
{
    return gc_state.last_error;
}
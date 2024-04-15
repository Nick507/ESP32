/*
  generic_map.h - driver code for ESP32

  Part of grblHAL

  Copyright (c) 2020-2023 Terje Io

  grblHAL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  grblHAL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with grblHAL. If not, see <http://www.gnu.org/licenses/>.
*/

#define BOARD_NAME "MY BOARD"

#if N_ABC_MOTORS > 0
#error "Axis configuration is not supported!"
#endif

// timer definitions
#define STEP_TIMER_GROUP        TIMER_GROUP_0
#define STEP_TIMER_INDEX        TIMER_0

// Define step pulse output pins.
#define X_STEP_PIN              GPIO_NUM_32
#define Y_STEP_PIN              GPIO_NUM_14
#define Z_STEP_PIN              GPIO_NUM_25

// Define step direction output pins. NOTE: All direction pins must be on the same port.
#define X_DIRECTION_PIN         GPIO_NUM_33
#define Y_DIRECTION_PIN         GPIO_NUM_12
#define Z_DIRECTION_PIN         GPIO_NUM_26

// Define stepper driver enable/disable output pin(s).
#define STEPPERS_ENABLE_PIN     GPIO_NUM_27
//#define Y_ENABLE_PIN          GPIO_NUM_13

#define HAND_ENCODER_CH_A_PIN   GPIO_NUM_34
#define HAND_ENCODER_CH_B_PIN   GPIO_NUM_35

#if SDCARD_ENABLE
// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#define PIN_NUM_MISO            GPIO_NUM_19
#define PIN_NUM_MOSI            GPIO_NUM_23
#define PIN_NUM_CLK             GPIO_NUM_18
#define PIN_NUM_CS              GPIO_NUM_2
#endif

// display ports
#define TFT_CS_PIN      GPIO_NUM_5
#define TFT_RST_PIN     GPIO_NUM_21
#define TFT_DC_PIN      GPIO_NUM_22
#define TFT_MOSI_PIN    GPIO_NUM_23
#define TFT_SCK_PIN     GPIO_NUM_18
#define TFT_MISO_PIN    GPIO_NUM_19

#define TOUCH_CS_PIN    GPIO_NUM_15
#define TOUCH_CLK_PIN   TFT_SCK_PIN   // shared
#define TOUCH_DIN_PIN   TFT_MOSI_PIN  // shared
#define TOUCH_DOUT_PIN  TFT_MISO_PIN  // shared

// spindle vfd modbus
#define UART2_RX_PIN            GPIO_NUM_16
#define UART2_TX_PIN            GPIO_NUM_17
#define MODBUS_DIR_AUX          0
#define MODBUS_DIRECTION_PIN    GPIO_NUM_4

// free: GPIO13, GPIO2 (output only!), TFT_RST (move to hw)?
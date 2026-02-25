/*
  btt_skr_mini_e3_2.0_map.h - driver code for STM32F103RC ARM processors

  Part of grblHAL

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

#ifndef STM32F103xE
#error "This board has a STM32F10RCT6 processor, select a corresponding build!"
#endif

#define BOARD_NAME "Sovol SV06 Plus"

#define HAS_BOARD_INIT 1
#define SERIAL_PORT 1
#define SERIAL1_PORT 3

// Define step pulse output pins.
#define X_STEP_PORT           GPIOC
#define X_STEP_PIN            2  //PC2

#define Y_STEP_PORT           GPIOB
#define Y_STEP_PIN            8  //PB8

#define Z_STEP_PORT           GPIOB
#define Z_STEP_PIN            6  //PB6

#define STEP_OUTMODE            GPIO_BITBAND

// Define step direction output pins.
#define X_DIRECTION_PORT        GPIOB
#define X_DIRECTION_PIN         9 //PB9
#define Y_DIRECTION_PORT        GPIOB
#define Y_DIRECTION_PIN         7  //PB7
#define Z_DIRECTION_PORT        GPIOB
#define Z_DIRECTION_PIN         5  //PC5

#define DIRECTION_OUTMODE       GPIO_BITBAND

// Define stepper driver enable/disable output pin.
#define X_ENABLE_PORT           GPIOC
#define X_ENABLE_PIN            3 //PC3
#define Y_ENABLE_PORT           GPIOC
#define Y_ENABLE_PIN            3 //PC3
#define Z_ENABLE_PORT           GPIOC
#define Z_ENABLE_PIN            3  //PB3

// Define homing/hard limit switch input pins.
#define LIMIT_PORT              GPIOA
#define X_LIMIT_PIN             5 //PA5
#define Y_LIMIT_PIN             6 //PA6
#define Z_LIMIT_PIN             15 // not used
#define Z_LIMIT_BIT             0 // not used
#define LIMIT_INMODE            GPIO_BITBAND

#define AUXOUTPUT1_PORT         GPIOA // Fan
#define AUXOUTPUT1_PIN          0

// Define driver spindle pins
#if DRIVER_SPINDLE_ENABLE & SPINDLE_ENA
#define SPINDLE_ENABLE_PORT     GPIOA
#define SPINDLE_ENABLE_PIN      15
#define SPINDLE_ENABLE_BIT      0
#endif
#if DRIVER_SPINDLE_ENABLE & SPINDLE_PWM
#define SPINDLE_PWM_PORT_BASE   GPIOB_BASE
#define SPINDLE_PWM_PORT        GPIOB
#define SPINDLE_PWM_PIN         0
#endif
#if DRIVER_SPINDLE_ENABLE & SPINDLE_DIR
#define SPINDLE_DIRECTION_PORT  GPIOA
#define SPINDLE_DIRECTION_PIN   15
#define SPINDLE_DIRECTION_BIT   0
#endif

// Define flood and mist coolant enable output pins.
#define COOLANT_FLOOD_PORT      AUXOUTPUT1_PORT
#define COOLANT_FLOOD_PIN       AUXOUTPUT1_PIN

// Define user-control controls (cycle start, reset, feed hold) input pins.

#if PROBE_ENABLE
#define PROBE_PORT              GPIOB
#define PROBE_PIN               1
#endif

#if SDCARD_ENABLE

#define SDCARD_SDIO           1
#define SD_CS_PORT            GPIOA
#define SD_CS_PIN             4

#endif
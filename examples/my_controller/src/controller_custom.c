/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2019 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * hello_world.c - App layer application of a simple hello world debug print every
 *   2 seconds.
 */


#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "app.h"

#include "FreeRTOS.h"
#include "task.h"

#include "controller.h"
#include "controller_custom.h"
#include "controller_pid.h"

#define DEBUG_MODULE "CUSTOMCONTROLLER"
#include "debug.h"


void appMain() {
  DEBUG_PRINT("Waiting for activation ...\n");

  while(1) {
    vTaskDelay(M2T(2000));
    
    // Disable DEBUG_PRINT
    // DEBUG_PRINT("Hello World!\n");
  }
}

void controllerCustomInit() {
  // Initialize controller data 

  // Call PID controller temporarily
  controllerPidInit():
}

bool controllerCustomInit() {
  // Always return true
  return true;
}

void controllerCustom(control_t *control, 
                      const setpoint_t *setpoint, 
                      const sensorData_t *sensors, 
                      const state_t *state, 
                      const uint32_t tick) {
  // Implement controller here

  // Call PID controller temporarily
  controllerPid(control, setpoint, sensors, state, tick);

}

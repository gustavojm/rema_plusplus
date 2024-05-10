//#include <stdlib.h>
#include <chrono>
#include <cstdint>
#include <new>

#include "FreeRTOS.h"
#include "board.h"
#include "bresenham.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "xy_axes.h"

#include "debug.h"
#include "gpio.h"
#include "tmr.h"

bresenham *x_y_axes = nullptr;

/**
 * @brief   initializes the stepper motors for bresenham control
 * @returns	nothing
 */
bresenham &xy_axes_init() {
  static mot_pap x_axis('X');
  static mot_pap y_axis('Y');
  static tmr xy_axes_tmr =
      tmr(LPC_TIMER0, RGU_TIMER0_RST, CLK_MX_TIMER0, TIMER0_IRQn);
  alignas(bresenham) static char xy_axes_buf[sizeof(bresenham)];

  x_axis.motor_resolution = 25000;
  x_axis.encoder_resolution = 500;
  x_axis.inches_to_counts_factor = 5000;
  x_axis.reversed = true;

  x_axis.gpios.step = gpio{4, 8, SCU_MODE_FUNC4, 5, 12}
                          .init_output(); // DOUT4 P4_8    PIN15   GPIO5[12]

  y_axis.motor_resolution = 25000;
  y_axis.encoder_resolution = 500;
  y_axis.inches_to_counts_factor = 5000;

  y_axis.gpios.step = gpio{4, 9, SCU_MODE_FUNC4, 5, 13}
                          .init_output(); // DOUT5 P4_9    PIN33   GPIO5[13]

  x_y_axes = new (xy_axes_buf)
      bresenham("xy_axes", &x_axis, &y_axis, xy_axes_tmr, true);
  x_y_axes->kp = {
      100,                 //!< Kp
      x_y_axes->step_time, //!< Update rate (ms)
      // 10000,                                  //!< Min output
      // 100000                                  //!< Max output
      500, //!< Min output             LOWER TIMER SETTINGS FOR ENCODER-MOTOR
           //!< SIMULATOR
      5000 //!< Max output
  };

  return *x_y_axes;
}

/**
 * @brief   handle interrupt from 32-bit timer to generate pulses for the
 * stepper motor drivers
 * @returns nothing
 * @note    calls the supervisor task every x number of generated steps
 */
extern "C" void TIMER0_IRQHandler(void) {
  if (x_y_axes->tmr.match_pending()) {
    x_y_axes->isr();
  }
}

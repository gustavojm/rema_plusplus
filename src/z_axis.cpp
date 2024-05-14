//#include <stdlib.h>
#include <cstdint>
#include <new>

#include "FreeRTOS.h"
#include "board.h"
#include "bresenham.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "z_axis.h"

#include "debug.h"
#include "gpio.h"
#include "tmr.h"

bresenham *z_dummy_axes = nullptr;

/**
 * @brief 	initializes the stepper motors for bresenham control
 * @returns	nothing
 */
bresenham &z_axis_init() {
  static mot_pap z_axis('Z');
  static mot_pap dummy_axis('D', true);
  static tmr z_dummy_axes_tmr =
      tmr(LPC_TIMER1, RGU_TIMER1_RST, CLK_MX_TIMER1, TIMER1_IRQn);
  alignas(bresenham) static char z_dummy_axes_buf[sizeof(bresenham)];

  z_axis.motor_resolution = 25000;
  z_axis.encoder_resolution = 500;
  z_axis.inches_to_counts_factor = 5000;

  z_axis.gpios.step = gpio{4, 10, SCU_MODE_FUNC4, 5, 14}
                          .init_output(); // DOUT6 P4_10   PIN35   GPIO5[14]

  z_dummy_axes = new (z_dummy_axes_buf)
      bresenham("z_dummy_axes", &z_axis, &dummy_axis, z_dummy_axes_tmr);
  z_dummy_axes->kp = {
      100,                     //!< Kp
      z_dummy_axes->step_time, //!< Update rate (ms)
      10000,                   //!< Min output
      100000                   //!< Max output
  };

  return *z_dummy_axes;
}

/**
 * @brief   handle interrupt from 32-bit timer to generate pulses for the
 * stepper motor drivers
 * @returns nothing
 * @note    calls the supervisor task every x number of generated steps
 */
extern "C" void TIMER1_IRQHandler(void) {
  if (z_dummy_axes->tmr.match_pending()) {
    z_dummy_axes->isr();
  }
}

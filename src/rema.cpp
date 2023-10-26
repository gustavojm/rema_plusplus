#include "rema.h"
#include "board.h"
#include "gpio.h"

gpio_templ< 4, 4, SCU_MODE_FUNC0, 2, 4 >  relay_4;      // DOUT3 P4_4    PIN9    GPIO2[4]>

bool rema::control_enabled = false;
bool rema::probe_enabled = false;
bool rema::stall_detection = true;
TickType_t rema::lastKeepAliveTicks;

struct rema::hard_limits rema::hard_limits;
rema::palper_t rema::palper;

void rema::control_enabled_set(bool status) {
    control_enabled = status;
}

bool rema::control_enabled_get() {
    return control_enabled;
}

void rema::probe_enabled_set(bool status)  {
    probe_enabled = status;
}

bool rema::probe_enabled_get()  {
    return probe_enabled;
}

void rema::stall_control_set(bool status) {
    stall_detection = status;
}

bool rema::stall_control_get() {
    return stall_detection;
}

void rema::lamp_pwr_set(bool status) {
    relay_4.init_output();
    relay_4.set(status);
}

void rema::update_watchdog_timer() {
    lastKeepAliveTicks = xTaskGetTickCount();
}

bool rema::is_watchdog_expired() {
    return ((xTaskGetTickCount() - lastKeepAliveTicks) > pdMS_TO_TICKS(WATCHDOG_TIME_MS) );
}


extern bresenham x_y_axes;
extern bresenham z_dummy_axes;

void rema::hard_limits_init(void)
{
    {
        palper.init_input();
        hard_limits.z_out.init_input();
        hard_limits.z_in.init_input();

        /* Group GPIO interrupt 0 */
        int gpio_port = 2;
        int bitmask = 1 << hard_limits.z_out.gpio_bit_ | 1 << hard_limits.z_in.gpio_bit_;

        Chip_GPIOGP_SelectHighLevel(LPC_GPIOGROUP, 0, gpio_port, bitmask);
        Chip_GPIOGP_EnableGroupPins(LPC_GPIOGROUP, 0, gpio_port, bitmask);
        Chip_GPIOGP_SelectOrMode(LPC_GPIOGROUP, 0);
        Chip_GPIOGP_SelectEdgeMode(LPC_GPIOGROUP, 0);

        /* Enable Group GPIO interrupt 0 */
        //NVIC_EnableIRQ(GINT0_IRQn);
    }

    {
        hard_limits.y_up.init_input();
        hard_limits.y_down.init_input();
        hard_limits.x_right.init_input();
        hard_limits.x_left.init_input();


        /* Group GPIO interrupt 1 */
        int gpio_port = 3;
        int bitmask = 1 << hard_limits.y_up.gpio_bit_ | 1 << hard_limits.y_down.gpio_bit_ | 1 << hard_limits.x_right.gpio_bit_ | 1 << hard_limits.x_left.gpio_bit_;

        /* Group GPIO interrupt 1 will be invoked when the button is pressed. */
        Chip_GPIOGP_SelectLowLevel(LPC_GPIOGROUP, 1, gpio_port, bitmask);
        Chip_GPIOGP_EnableGroupPins(LPC_GPIOGROUP, 1, gpio_port, bitmask);
        Chip_GPIOGP_SelectOrMode(LPC_GPIOGROUP, 1);
        Chip_GPIOGP_SelectEdgeMode(LPC_GPIOGROUP, 1);

        /* Enable Group GPIO interrupt 1 */
        //NVIC_EnableIRQ(GINT1_IRQn);
    }
}


static void hard_limits_reached() {
    /* TODO Read input pins to determine which limit has been reached and stop only one motor*/
    z_dummy_axes_get_instance().stop();
    x_y_axes_get_instance().stop();
}

/**
* @brief    Handle Grouped Interrupt from GPIO pin or GPIO pin mapped to PININT
* @return   Nothing
*/
extern "C" void GINT0_IRQHandler(void) {
    hard_limits_reached();
    Chip_GPIOGP_ClearIntStatus(LPC_GPIOGROUP, 0);
}

extern "C" void GINT1_IRQHandler(void) {
    hard_limits_reached();
    Chip_GPIOGP_ClearIntStatus(LPC_GPIOGROUP, 1);
}

extern "C" void GPIO3_IRQHandler(void)
{
    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(3));
//    x_axis.save_probe_pos_and_stop();
//    y_axis.save_probe_pos_and_stop();
//    z_axis.save_probe_pos_and_stop();
}

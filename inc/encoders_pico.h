#ifndef ENCODERS_PICO_H_
#define ENCODERS_PICO_H_

#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "board.h"
#include "spi.h"
#include "gpio_templ.h"
#include "gpio.h"
#include "debug.h"

/* Control register bits description */
#define ENCODERS_PICO_CLEAR_COUNTERS     0x20
#define ENCODERS_PICO_CLEAR_COUNTER_X    ENCODERS_PICO_CLEAR_COUNTERS + 1
#define ENCODERS_PICO_CLEAR_COUNTER_Y    ENCODERS_PICO_CLEAR_COUNTERS + 2
#define ENCODERS_PICO_CLEAR_COUNTER_Z    ENCODERS_PICO_CLEAR_COUNTERS + 3
#define ENCODERS_PICO_CLEAR_COUNTER_W    ENCODERS_PICO_CLEAR_COUNTERS + 4

#define ENCODERS_PICO_COUNTERS          0x30
#define ENCODERS_PICO_COUNTER_X         ENCODERS_PICO_COUNTERS + 1
#define ENCODERS_PICO_COUNTER_Y         ENCODERS_PICO_COUNTERS + 2
#define ENCODERS_PICO_COUNTER_Z         ENCODERS_PICO_COUNTERS + 3
#define ENCODERS_PICO_COUNTER_W         ENCODERS_PICO_COUNTERS + 4

#define ENCODERS_PICO_TARGETS           0x40
#define ENCODERS_PICO_TARGET_X          ENCODERS_PICO_TARGETS + 1
#define ENCODERS_PICO_TARGET_Y          ENCODERS_PICO_TARGETS + 2
#define ENCODERS_PICO_TARGET_Z          ENCODERS_PICO_TARGETS + 3
#define ENCODERS_PICO_TARGET_W          ENCODERS_PICO_TARGETS + 4

#define ENCODERS_POS_THRESHOLDS         0x50
#define ENCODERS_POS_THRESHOLD_X        ENCODERS_POS_THRESHOLDS + 1
#define ENCODERS_POS_THRESHOLD_Y        ENCODERS_POS_THRESHOLDS + 2
#define ENCODERS_POS_THRESHOLD_Z        ENCODERS_POS_THRESHOLDS + 3
#define ENCODERS_POS_THRESHOLD_W        ENCODERS_POS_THRESHOLDS + 4

#define ENCODERS_HARD_LIMITS            0X60
#define ENCODERS_SOFT_LIMITS            0X70

#define ENCODERS_PICO_SOFT_RESET        0XFF

#define ENCODERS_PICO_TASK_PRIORITY     ( configMAX_PRIORITIES - 1)
#define ENCODERS_PICO_INTERRUPT_PRIORITY  (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1)   //Has to have higher priority than timers ( now +2 )

extern SemaphoreHandle_t encoders_pico_semaphore;

extern gpio_templ< 4, 4, SCU_MODE_FUNC0, 2, 4 > click;      // DOUT3 P4_4    PIN9    GPIO2[4]>

/**
 * @brief   handles the CS line for the ENCODERS RASPBERRY PI PICO
 * @param   state    : boolean value for the output
 * @returns nothing
 * @note    the falling edge of CS takes the SDI and SDO lines out of the high impedance state???
 */
static void cs_function(bool state) {
    if (state) {
        Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 5, 15);
    } else {
        Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 5, 15);
    }
}

/**
 * @struct     encoders_pico
 * @brief    Raspberry-pi Pico device instance specific state.
 */
class encoders_pico {
public:
    encoders_pico() {
        Chip_SCU_PinMuxSet(6, 7, (SCU_MODE_PULLUP | SCU_MODE_FUNC4));  // GPIO3 P6_7    PIN85    GPIO5[15] CS ENCODER RASPBERRY PI PICO
        Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 5, 15);
        Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 5, 15);
        spi_init();
    }

    ~encoders_pico() {
        spi_de_init();
    }

    static void task(void *pars) {    
        while (true) {
            if (xSemaphoreTake(encoders_pico_semaphore, portMAX_DELAY) == pdPASS) {
                auto &encoders = encoders_pico::get_instance();                
                lDebug(Info, "hard_limits: %d \n", encoders.read_hard_limits());
                vTaskDelay(pdMS_TO_TICKS(1000));
                encoders.ack_hard_limits();                
            }
        }
    }

    static void init() {
        gpio_pinint encoders_irq_pin = {6, 1, (SCU_MODE_INBUFF_EN | SCU_MODE_PULLDOWN | SCU_MODE_FUNC0), 3, 0, PIN_INT0_IRQn};   //GPIO5 P6_1     PIN74   GPIO3[0]    
        encoders_irq_pin.init_input()
                .mode_edge()
                .int_high()
                .clear_pending()
                .enable();
    
        NVIC_SetPriority(PIN_INT0_IRQn, ENCODERS_PICO_INTERRUPT_PRIORITY);

        encoders_pico_semaphore = xSemaphoreCreateBinary();

        if (encoders_pico_semaphore != NULL) {
            // Create the 'handler' task, which is the task to which interrupt processing is deferred
            xTaskCreate(encoders_pico::task, "encoders_pico",
            256,
            NULL, ENCODERS_PICO_TASK_PRIORITY, NULL);
            lDebug(Info, "encoders_pico_task created");
        }

    }

    static encoders_pico& get_instance() {
        static encoders_pico instance;
        return instance;
    }

    int32_t read_register(uint8_t address) const;

    void read_3_registers(uint8_t address, uint8_t *rx) const;

    int32_t write_register(uint8_t address, int32_t data) const;

    int32_t soft_reset() const;

    uint8_t read_hard_limits() const;

    void ack_hard_limits() const;

public:
    void (*cs)(bool) = cs_function;             ///< pointer to CS line function handler
};

#endif /* ENCODERS_PICO_H_ */


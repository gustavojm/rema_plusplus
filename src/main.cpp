#include <chrono>
#include <cstdint>
#include <signal.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

/* LWIP and ENET phy */
#include "arch/lpc18xx_43xx_emac.h"
#include "arch/lpc_arch.h"
#include "arch/sys_arch.h"
#include "lpc_phy.h" /* For the PHY monitor support */

/* LPC */
#include "board.h"

#include "debug.h"
#include "encoders_pico.h"
#include "lwip/ip_addr.h"
#include "lwip_init.h"
#include "mem_check.h"
#include "mot_pap.h"
#include "rema.h"
#include "settings.h"
#include "temperature_ds18b20.h"
#include "xy_axes.h"
#include "z_axis.h"

/* GPa 201117 1850 Iss2: agregado de Heap_4.c*/
uint8_t __attribute__((section("."
                               "data"
                               ".$"
                               "RamAHB32_48_64"))) ucHeap[configTOTAL_HEAP_SIZE];

/* Sets up system hardware */
static void prvSetupHardware(void) {
#if defined(__FPU_PRESENT) && __FPU_PRESENT == 1
    fpuInit();
#endif

    Board_SystemInit();
    SystemCoreClockUpdate();

    // Without the following command the use of DWT makes debugging impossible
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // Enable Cycle Counter, used to create precision delays in wait.c
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    Board_Init();
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief    MilliSecond delay function based on FreeRTOS
 * @param    ms    : Number of milliSeconds to delay
 * @returns    nothing
 * Needed for some functions, do not use prior to FreeRTOS running
 */
void msDelay(uint32_t ms) {
    vTaskDelay((configTICK_RATE_HZ * ms) / 1000);
}

/**
 * @brief    main routine for example_lwip_tcpecho_freertos_18xx43xx
 * @returns    function should not exit
 */
int main(void) {
    debugInit();
    debugLocalSetLevel(true, Info);
    debugNetSetLevel(true, Debug);

    prvSetupHardware();

    // printf("    --- NASA GSPC ---\n");
    // printf("REMA Remote Terminal Unit.\n");

    settings::init();

    rema::init_outputs();
    xy_axes_init();
    z_axis_init();
    encoders_pico_init();

    temperature_ds18b20_init();
    // mem_check_init();

    network_init();

    /* Start the scheduler itself. main()'s stack gets destroyed at this point */
    vTaskStartScheduler();

    return 1;
}

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
extern "C" void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
    volatile signed char *name;
    volatile xTaskHandle *pxT;

    name = pcTaskName;
    pxT = pxTask;

    (void)name;
    (void)pxT;

    while (1) {
    }
}
#endif

/*-----------------------------------------------------------*/
/**
 * @brief     configASSERT callback function
 * @param     ulLine        : line where configASSERT was called
 * @param     pcFileName    : file where configASSERT was called
 */
void vAssertCalled(uint32_t ulLine, const char *const pcFileName) {
    volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

    taskENTER_CRITICAL();
    {
        printf("[ASSERT] %s:%lu\n", pcFileName, ulLine);
        /* You can step out of this function to debug the assertion by using
         the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
         value. */
        while (ulSetToNonZeroInDebuggerToContinue == 0) {
        }
    }
    taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

extern "C" void prvGetRegistersFromStack(uint32_t *pulFaultStackAddress) {
    /* These are volatile to try and prevent the compiler/linker optimising them
     away as the variables never actually get used.  If the debugger won't show
     the values of the variables, make them global my moving their declaration
     outside of this function. */
    volatile uint32_t r0;
    volatile uint32_t r1;
    volatile uint32_t r2;
    volatile uint32_t r3;
    volatile uint32_t r12;
    volatile uint32_t lr;  /* Link register. */
    volatile uint32_t pc;  /* Program counter. */
    volatile uint32_t psr; /* Program status register. */

    r0 = pulFaultStackAddress[0];
    r1 = pulFaultStackAddress[1];
    r2 = pulFaultStackAddress[2];
    r3 = pulFaultStackAddress[3];

    r12 = pulFaultStackAddress[4];
    lr = pulFaultStackAddress[5];
    pc = pulFaultStackAddress[6];
    psr = pulFaultStackAddress[7];

    (void)r0;
    (void)r1;
    (void)r2;
    (void)r3;
    (void)r12;
    (void)lr;
    (void)pc;
    (void)psr;

    /* When the following line is hit, the variables contain the register values.
     */
    for (;;) {
    }
}

//__attribute__((section(".after_vectors"))) void HardFault_Handler(void) {
void HardFault_Handler(void) {
    while (1) {
        gpio_templ<4, 4, SCU_MODE_FUNC0, 2, 4> relay_DOUT3; // DOUT3 P4_4    PIN9    GPIO2[4] Bornes 10 y 11
        relay_DOUT3.set(1);
        udelay(1000000);
        relay_DOUT3.set(0);
        udelay(1000000);
    }
}
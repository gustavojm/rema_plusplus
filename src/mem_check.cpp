//#include <stdlib.h>

#include "mem_check.h"
#include "board.h"
#include "debug.h"

#define MEM_CHECK_TASK_PRIORITY (0)

static void mem_check_task(void *par) {
    while (true) {
        vPortCheckIntegrity();
        vPortMemoryScan();

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief 	initializes ADC to read temperature sensor.
 * @return	nothing
 */
void mem_check_init() {
    xTaskCreate(mem_check_task, "MemCheck", configMINIMAL_STACK_SIZE * 2, NULL, MEM_CHECK_TASK_PRIORITY, NULL);
    lDebug(Info, "MemCheck: task created");
}

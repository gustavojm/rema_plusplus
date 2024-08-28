#include "debug.h"

#include <stdio.h>
#include <string.h>

void uart_DMA_TX_task([[maybe_unused]] void *pars) {
    while (true) {           
        static char *old_debug_msg = nullptr;  
        char *debug_msg;  
        if (xQueueReceive(uart_debug_queue, &debug_msg, portMAX_DELAY) == pdPASS) {
            size_t msg_len = strlen(debug_msg);
            if (msg_len > 0) {                
                if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == pdPASS) {
                    if (old_debug_msg) {        // If we are here means that last DMA transfer ended.
                        delete(old_debug_msg);
                    }
                    Chip_GPDMA_Transfer(LPC_GPDMA,
                        0,                                     // DMA 0 channel
                        (uint32_t)debug_msg,                   // Source memory address
                        GPDMA_CONN_UART2_Tx,                   // Dirección del registro THR del USART
                        GPDMA_TRANSFERTYPE_M2P_CONTROLLER_DMA, // Memory to peripheral, DMA control
                        msg_len                                // Transfer size
                    );  
                    old_debug_msg = debug_msg;  
                }
            }
        }
    }
}

void debugInit() {
    uart_mutex = xSemaphoreCreateMutex();
    uart_debug_queue = xQueueCreate(UART_DEBUG_QUEUE_SIZE, sizeof(char *));

    network_mutex = xSemaphoreCreateMutex();
    network_debug_queue = xQueueCreate(NET_DEBUG_QUEUE_SIZE, sizeof(char *));

    // Enable DMA clock
    Chip_GPDMA_Init(LPC_GPDMA);

	/* Setting GPDMA interrupt */
	NVIC_DisableIRQ(DMA_IRQn);
	NVIC_SetPriority(DMA_IRQn, UART_DMA_TX_INTERRUPT_PRIORITY);
	NVIC_EnableIRQ(DMA_IRQn);

    xTaskCreate(uart_DMA_TX_task, "uart_DMA_TX", 256, NULL, UART_DMA_TX_TASK_PRIORITY, &uart_DMA_TX_task_handle);
    xTaskNotifyGive(uart_DMA_TX_task_handle);      // First notification, the following will be given by DMA Interrupt handler
}

/**
 * @brief 	sets local debug level.
 * @param 	enable 	:whether or not it will be sent to UART
 * @param 	lvl 	:minimum level to print
 */
void debugLocalSetLevel(bool enable, enum debugLevels lvl) {
    debug_to_uart = enable;
    debugLocalLevel = lvl;
}

/**
 * @brief 	sets debug level to send to network.
 * @param 	enable 	:whether or not it will be sent to network
 * @param 	lvl 	:minimum level to print
 */
void debugNetSetLevel(bool enable, enum debugLevels lvl) {
    debug_to_network = enable;
    debugNetLevel = lvl;
}

/**
 * @brief sends debugging output to a file.
 * @param fileName name of file to send output to
 */
void debugToFile(const char *fileName) {
    debugClose();

    FILE *file = fopen(fileName, "w"); // "w+" ?

    if (file) {
        debugFile = file;
    }
}

/** Close the output file if it was set in <tt>toFile()</tt> */
void debugClose() {
    if (debugFile && (debugFile != stderr)) {
        fclose(debugFile);
        debugFile = stderr;
    }
}

extern "C" void DMA_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Verificar qué canal de DMA generó la interrupción
    if (Chip_GPDMA_IntGetStatus(LPC_GPDMA, GPDMA_STAT_INTTC, 0)) {
        // Manejar la interrupción por finalización de la transferencia (Tx)
        Chip_GPDMA_ClearIntPending(LPC_GPDMA, GPDMA_STATCLR_INTTC, 0);  // Limpiar la interrupción
    
        vTaskNotifyGiveFromISR(uart_DMA_TX_task_handle, &xHigherPriorityTaskWoken);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

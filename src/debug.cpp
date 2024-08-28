#include "debug.h"

#include <stdio.h>
#include <string.h>

void uart_DMA_Transfer(char* txBuffer, uint32_t len) {
    Chip_GPDMA_Transfer(LPC_GPDMA,
                            0,                                     // Canal de DMA 0
                            (uint32_t)txBuffer,                    // Dirección de la memoria fuente
                            GPDMA_CONN_UART2_Tx,            // Dirección del registro THR del USART
                            GPDMA_TRANSFERTYPE_M2P_CONTROLLER_DMA, // Memoria a periférico DMA CONTROL
                            len                                    // Tamaño de la transferencia
                        );    

}

void debugInit() {
    uart_mutex = xSemaphoreCreateMutex();
    network_mutex = xSemaphoreCreateMutex();
    debug_queue = xQueueCreate(NET_DEBUG_QUEUE_SIZE, sizeof(char *));
    uart_queue = xQueueCreate(NET_DEBUG_QUEUE_SIZE, sizeof(char *));
    uart_DMA_task_init();
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

void uart_DMA_task([[maybe_unused]] void *pars) {
    while (true) {           
        char *debug_msg;  
        if (xQueueReceive(uart_queue, &debug_msg, portMAX_DELAY) == pdPASS) {
            size_t msg_len = strlen(debug_msg);
            if (msg_len > 0) {                    
                msg_len++;
                if (xSemaphoreTake(uart_DMA_semaphore, portMAX_DELAY) == pdPASS) {
                    uart_DMA_Transfer(debug_msg, msg_len);
                }
            }
        }
    }
}

extern "C" void DMA_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Verificar qué canal de DMA generó la interrupción
    if (Chip_GPDMA_IntGetStatus(LPC_GPDMA, GPDMA_STAT_INTTC, 0)) {
        // Manejar la interrupción por finalización de la transferencia (Tx)
        Chip_GPDMA_ClearIntPending(LPC_GPDMA, GPDMA_STATCLR_INTTC, 0);  // Limpiar la interrupción
    
        xSemaphoreGiveFromISR(uart_DMA_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void uart_DMA_task_init() {
    uart_DMA_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(uart_DMA_semaphore);                 // When no DMA transfers occured yet, allow making the first one

   // Habilitar el reloj para el DMA
    Chip_GPDMA_Init(LPC_GPDMA);
 
	/* Setting GPDMA interrupt */
	NVIC_DisableIRQ(DMA_IRQn);
	NVIC_SetPriority(DMA_IRQn, UART_DMA_INTERRUPT_PRIORITY);
	NVIC_EnableIRQ(DMA_IRQn);

    if (uart_DMA_semaphore != NULL) {
        // Create the 'handler' task, which is the task to which interrupt
        // processing is deferred
        xTaskCreate(uart_DMA_task, "uart_DMA", 256, NULL, UART_DMA_TASK_PRIORITY, NULL);
    }
}

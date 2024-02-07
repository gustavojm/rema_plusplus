#include "spi.h"

#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "board.h"

static SSP_ConfigFormat ssp_format;
static SemaphoreHandle_t spi_mutex;

/**
 * \brief 	executes SPI transfers synchronized by CS.
 * @param 	xfers			: pointer to array of transfers Chip_SSP_DATA_SETUP_T
 * @param 	num_xfers		: transfers count
 * @param 	cs	: pointer to WR/FSYNC line function handler
 * @returns	0 on success
 * @note 	this function takes a mutex to avoid interleaving transfers to both RDCs.
 * 			could work without mutex but debugging with a logic analyzer would be more confusing.
 */
int32_t spi_sync_transfer(Chip_SSP_DATA_SETUP_T *xfers, uint32_t num_xfers,
        void (*cs)(bool)) {
    uint32_t i;
    if (spi_mutex != NULL) {
        if (xSemaphoreTake(spi_mutex, portMAX_DELAY) == pdTRUE) {
            for (i = 0; i < num_xfers; ++i) {
                if (cs != NULL) {
                    cs(0);
                }
                udelay(100);
                Chip_SSP_RWFrames_Blocking(LPC_SSP, &(xfers[i]));
                udelay(100);
                if (cs != NULL) {
                    cs(1);
                }
                //udelay(100);
                if (i != num_xfers) {
                    /* Delay WR/FSYNC falling edge to SCLK rising edge 3 ns min
                     Delay WR/FSYNC falling edge to SDO release from high-Z
                     VDRIVE = 4.5 V to 5.25 V 16 ns min */
                    // udelay(10);
                }
            }
            xSemaphoreGive(spi_mutex);
        }
    }
    return 0;
}

/**
 * \brief 	initializes SSP bus to transfer SPI frames as a MASTER.
 * @returns	noting
 * @note 	sets SPI bitrate to 1Mhz SEE IF WE CAN IMPROVE THAT.
 */
void spi_init(void) {
    spi_mutex = xSemaphoreCreateMutex();

    Board_SSP_Init(LPC_SSP);
    Chip_SSP_Init(LPC_SSP);

    ssp_format.frameFormat = SSP_FRAMEFORMAT_SPI;
    ssp_format.bits = SSP_DATA_BITS;
    ssp_format.clockMode = SSP_CLOCK_MODE3;
    Chip_SSP_SetFormat(LPC_SSP, ssp_format.bits, ssp_format.frameFormat,
            ssp_format.clockMode);

    Chip_SSP_SetBitRate(LPC_SSP, 1000000);

    Chip_SSP_Enable(LPC_SSP);

    Chip_SSP_SetMaster(LPC_SSP, 1);
}

void spi_de_init(void) {
    vSemaphoreDelete(spi_mutex);

    Chip_SSP_DeInit(LPC_SSP);
}    
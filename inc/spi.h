#pragma once

#include <stdint.h>

#include "board.h"

#include "FreeRTOS.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LPC_SSP LPC_SSP1
#define SSP_DATA_BITS (SSP_BITS_8)

inline SemaphoreHandle_t encoders_mutex;

/**
 * \brief 	initializes SSP bus to transfer SPI frames as a MASTER.
 * @returns	noting
 * @note 	sets SPI bitrate to 1Mhz SEE IF WE CAN IMPROVE THAT.
 */
static inline void spi_init(void) {
  encoders_mutex = xSemaphoreCreateMutex();

  Board_SSP_Init(LPC_SSP);
  Chip_SSP_Init(LPC_SSP);

  static SSP_ConfigFormat ssp_format;
  ssp_format.frameFormat = SSP_FRAMEFORMAT_SPI;
  ssp_format.bits = SSP_DATA_BITS;
  ssp_format.clockMode = SSP_CLOCK_MODE3;
  Chip_SSP_SetFormat(LPC_SSP, ssp_format.bits, ssp_format.frameFormat,
                     ssp_format.clockMode);

  Chip_SSP_SetBitRate(LPC_SSP, 5000 * 1000);
  Chip_SSP_Enable(LPC_SSP);
  Chip_SSP_SetMaster(LPC_SSP, 1);
}

static inline void spi_de_init(void) {
  vSemaphoreDelete(encoders_mutex);

  Chip_SSP_DeInit(LPC_SSP);
}

/**
 * \brief 	executes SPI transfers synchronized by CS.
 * @param 	xfers			: pointer to array of transfers
 * Chip_SSP_DATA_SETUP_T
 * @param 	num_xfers		: transfers count
 * @param 	cs	: pointer to WR/FSYNC line function handler
 * @returns	0 on success
 * @note 	this function takes a mutex to avoid interleaving transfers to
 * both RDCs. could work without mutex but debugging with a logic analyzer would
 * be more confusing.
 */
static inline int32_t spi_sync_transfer(Chip_SSP_DATA_SETUP_T *xfer_setup,
                                        void (*cs)(bool) = nullptr) {
  if (cs != NULL) {
    cs(0);
  }
  udelay(2); // Si RPI PICO está haciendo printf para debug poner udelay(500)
  Chip_SSP_Int_FlushData(LPC_SSP);
  Chip_SSP_RWFrames_Blocking(LPC_SSP, xfer_setup);
  Chip_SSP_Int_FlushData(LPC_SSP);
  udelay(2); // Si RPI PICO está haciendo printf para debug poner udelay(500)
  if (cs != NULL) {
    cs(1);
  }
  udelay(2); // Si RPI PICO está haciendo printf para debug poner udelay(500)
  return 0;
}

/**
 * @brief     SPI synchronous write
 * @param     buf        : data buffer
 * @param     len        : data buffer size
 * @return    0 on success
 * @note     This function writes the buffer buf.
 */
static inline int spi_write(void *buf, size_t len, void (*cs)(bool)) {
  /* @formatter:off */
  Chip_SSP_DATA_SETUP_T t = {.tx_data = buf,
                             .length = static_cast<uint32_t>(len)};
  /* @formatter:on */
  return spi_sync_transfer(&t, cs);
}

/**
 * @brief     SPI synchronous read
 * @param     buf        : data buffer
 * @param     len        : data buffer size
 * @return     0 on success
 * @note     This function reads from SPI to the buffer buf.
 */
static inline int spi_read(void *buf, size_t len, void (*cs)(bool)) {
  /* @formatter:off */
  Chip_SSP_DATA_SETUP_T t = {.rx_data = buf,
                             .length = static_cast<uint32_t>(len)};
  /* @formatter:on */
  return spi_sync_transfer(&t, cs);
}

#ifdef __cplusplus
}
#endif


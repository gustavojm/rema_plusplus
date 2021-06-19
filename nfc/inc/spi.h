#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

void spi_init(void);

int32_t spi_sync_transfer(Chip_SSP_DATA_SETUP_T *xfers, uint32_t num_xfers,
        void (*gpio_wr_fsync)(bool));

/**
 * @brief     SPI synchronous write
 * @param     buf        : data buffer
 * @param     len        : data buffer size
 * @return    0 on success
 * @note     This function writes the buffer buf.
 */
static inline int spi_write(void *buf, size_t len,
        void (*gpio_wr_fsync)(bool)) {
    /* @formatter:off */
    Chip_SSP_DATA_SETUP_T t = {
             .tx_data = buf,
             .length = len
    };
    /* @formatter:on */

    return spi_sync_transfer(&t, 1, gpio_wr_fsync);
}

/**
 * @brief     SPI synchronous read
 * @param     buf        : data buffer
 * @param     len        : data buffer size
 * @return     0 on success
 * @note     This function reads from SPI to the buffer buf.
 */
static inline int spi_read(void *buf, size_t len, void (*gpio_wr_fsync)(bool)) {
    /* @formatter:off */
    Chip_SSP_DATA_SETUP_T t = {
             .rx_data = buf,
             .length = len
    };
    /* @formatter:on */

    return spi_sync_transfer(&t, 1, gpio_wr_fsync);
}

#ifdef __cplusplus
}
#endif

#endif /* SPI_H_ */

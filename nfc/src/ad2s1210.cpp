#include "ad2s1210.h"

#include <stdio.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "board.h"

#include "debug.h"
#include "spi.h"

static const uint32_t ad2s1210_resolution_value[] = { 10, 12, 14, 16 };

/**
 * @brief 	writes 1 byte (address or data) to the chip
 * @param 	me		: pointer to struct ad2s1210
 * @param 	data	: address or data to write through SPI
 * @returns	0 on success
 */
static int32_t ad2s1210_config_write(struct ad2s1210 *me, uint8_t data)
{
	int32_t ret = 0;

	uint8_t tx = data;
	ret = spi_write(&tx, 1, me->gpios.wr_fsync);

	return ret;
}

/**
 * @brief 	reads value from one of the registers of the chip in config mode (A0=1, A1=1)
 * @param 	me		: pointer to struct ad2s1210
 * @param 	address	: address to read through SPI
 * @returns	the value of the addressed record
 * @note	one extra register should be read because values come one transfer after the
 * 			address was put on the bus
 */
static uint8_t ad2s1210_config_read(struct ad2s1210 *me, uint8_t address)
{
	uint32_t xfers_count = 2;
	uint8_t rx[xfers_count];
	uint8_t tx[xfers_count];
	Chip_SSP_DATA_SETUP_T xfers[xfers_count];

	tx[0] = address | AD2S1210_ADDRESS_MASK;
	tx[1] = AD2S1210_REG_FAULT;				// Read some extra record to receive the data

	for (int i = 0; i < xfers_count; i++) {
		/* @formatter:off */
		Chip_SSP_DATA_SETUP_T xfer = {
			.tx_data = &tx[i],
			.tx_cnt = 0,
			.rx_data = &rx[i],
			.rx_cnt = 0,
			.length = 1
		};
	/* @formatter:on */

		xfers[i] = xfer;
	}
	spi_sync_transfer(xfers, xfers_count, me->gpios.wr_fsync);

	return rx[1];
}

/**
 * @brief 	reads value from two consecutive registers, used to read position and velocity.
 * @param 	st 		: struct ad2s1210
 * @param 	address	: address of the first register to read
 * @returns rx[1] << 8 | rx[2]
 * @note	one extra register should be read because values come one transfer after the
 * 			address was put on the bus.
 */
static uint16_t ad2s1210_config_read_two(struct ad2s1210 *me,
		uint8_t address)
{
	uint32_t xfers_count = 3;
	uint8_t rx[xfers_count];
	uint8_t tx[xfers_count];
	Chip_SSP_DATA_SETUP_T xfers[xfers_count];

	tx[0] = address | AD2S1210_ADDRESS_MASK;
	tx[1] = (address + 1) | AD2S1210_ADDRESS_MASK;
	tx[2] = AD2S1210_REG_FAULT;	// Read some extra record to receive the data

	for (int i = 0; i < xfers_count; i++) {
		/* @formatter:off */
		Chip_SSP_DATA_SETUP_T xfer = {
			.tx_data = &tx[i],
			.tx_cnt = 0,
			.rx_data = &rx[i],
			.rx_cnt = 0,
			.length = 1
		};
		/* @formatter:on */

		xfers[i] = xfer;
	}

	int32_t ret = 0;
	ret = spi_sync_transfer(xfers, xfers_count, me->gpios.wr_fsync);
	if (ret < 0)
		return ret;

	return rx[1] << 8 | rx[2];
}

/**
 * @brief 	updates frequency control word register.
 * @param 	me		: pointer to struct ad2s1210
 * @returns 0 on success
 * @returns -ERANGE if fcw < 0x4 or fcw > 0x50
 * @note	fcw = (fexcit * (2 exp 15)) / fclkin
 */
static int32_t ad2s1210_update_frequency_control_word(struct ad2s1210 *me)
{
	int32_t ret = 0;
	uint8_t fcw;

	fcw = (uint8_t) (me->fexcit * (1 << 15) / me->fclkin);
	if (fcw < AD2S1210_MIN_FCW || fcw > AD2S1210_MAX_FCW) {
		lDebug(Error, "ad2s1210: FCW out of range");
		return -1;
	}

	ret = ad2s1210_set_reg(me, AD2S1210_REG_EXCIT_FREQ, fcw);
	return ret;
}

/**
 * @brief 	soft resets the chip.
 * @param 	me		: pointer to struct ad2s1210
 * @returns	0 on success
 * @note	soft reset does not erase the configured values on the chip.
 */
int32_t ad2s1210_soft_reset(struct ad2s1210 *me)
{
	int32_t ret;

	ret = ad2s1210_config_write(me, AD2S1210_REG_SOFT_RESET);

	///\todo see if something must be written on the register for soft reset
	return ret;
}

/**
 * @brief 	hard resets the chip by lowering the RESET line.
 * @param 	me		: pointer to struct ad2s1210
 * @returns nothing
 */
void ad2s1210_hard_reset(struct ad2s1210 *me)
{
	me->gpios.reset(0);
	udelay(100);
	me->gpios.reset(1);
}

/**
 * @brief 	returns cached value of fclkin
 * @param 	me		: pointer to struct ad2s1210
 * @returns 	fclkin
 */
uint32_t ad2s1210_get_fclkin(struct ad2s1210 *me)
{
	return me->fclkin;
}

/**
 * @brief 	updates fclkin
 * @param 	me		: pointer to struct ad2s1210
 * @param 	fclkin	: XTAL frequency
 * @returns 0 on success
 * @returns	-1 if fclkin < 6144000 or fclkin > 10240000
 */
int32_t ad2s1210_set_fclkin(struct ad2s1210 *me, uint32_t fclkin)
{
	int32_t ret = 0;

	if (fclkin < AD2S1210_MIN_CLKIN || fclkin > AD2S1210_MAX_CLKIN) {
		lDebug(Error, "ad2s1210: fclkin out of range");
		return -1;
	}

	me->fclkin = fclkin;

	ret = ad2s1210_update_frequency_control_word(me);
	if (ret < 0)
		return ret;
	ret = ad2s1210_soft_reset(me);
	return ret;
}

/**
 * @brief 	returns cached value of fexcit
 * @param 	me		: pointer to struct ad2s1210
 * @returns 	fexcit
 */
uint32_t ad2s1210_get_fexcit(struct ad2s1210 *me)
{
	return me->fexcit;
}

/**
 * @brief 	sets excitation frequency of the resolvers
 * @param 	me		: pointer to struct ad2s1210
 * @param 	fexcit	: excitation frequency
 * @returns 0 on success
 * @returns	-1 if fexcit < 2000 or fexcit > 20000
 */
int32_t ad2s1210_set_fexcit(struct ad2s1210 *me, uint32_t fexcit)
{
	int32_t ret = 0;

	if (fexcit < AD2S1210_MIN_EXCIT || fexcit > AD2S1210_MAX_EXCIT) {
		lDebug(Error, "ad2s1210: excitation frequency out of range");
		return -1;
	}

	me->fexcit = fexcit;
	ret = ad2s1210_update_frequency_control_word(me);
	if (ret < 0)
		return ret;
	ret = ad2s1210_soft_reset(me);
	return ret;
}

/**
 * @brief 	gets control register.
 * @param 	me		: pointer to struct ad2s1210
 * @returns	0 on success
 */
static uint8_t ad2s1210_get_control(struct ad2s1210 *me)
{
	uint8_t ret = 0;

	ret = ad2s1210_config_read(me, AD2S1210_REG_CONTROL);
	return ret;
}

/**
 * @brief 	sets control register.
 * @param 	me		: pointer to struct ad2s1210
 * @param 	data	: value to set on the register
 * @returns	0 on success
 */
static int32_t ad2s1210_set_control(struct ad2s1210 *me, uint8_t data)
{
	int32_t ret = 0;

	int32_t udata = data & AD2S1210_DATA_MASK;
	ret = ad2s1210_set_reg(me, AD2S1210_REG_CONTROL, udata );
	if (ret < 0)
		return ret;

	ret = ad2s1210_get_reg(me, AD2S1210_REG_CONTROL);
	if (ret < 0)
		return ret;
	if (ret & AD2S1210_MSB_MASK) {
		ret = -1;
		lDebug(Error, "ad2s1210: write control register fail");
		return ret;
	}
	me->resolution = ad2s1210_resolution_value[data & AD2S1210_RESOLUTION_MASK];
	me->hysteresis = !!(data & AD2S1210_HYSTERESIS);
	return ret;
}

/**
 * @brief 	returns cached value of resolution.
 * @param 	me		: pointer to struct ad2s1210
 * @returns resolution
 */
uint8_t ad2s1210_get_resolution(struct ad2s1210 *me)
{
	return me->resolution;
}

/**
 * @brief 	sets value of resolution on the chip.
 * @param 	me		: pointer to struct ad2s1210
 * @param	res		: the desired resolution value. Possible values: 10, 12, 14, 16
 * @returns 0 on success
 * @returns -EINVAL if res < 10 or res > 16
 */
int32_t ad2s1210_set_resolution(struct ad2s1210 *me, uint8_t res)
{
	uint8_t data;
	int32_t ret = 0;

	if (res < 10 || res > 16) {
		lDebug(Error, "ad2s1210: resolution out of range");
		return -1;
	}

	ret = ad2s1210_get_control(me);
	if (ret < 0)
		return ret;
	data = ret;
	data &= ~AD2S1210_RESOLUTION_MASK;
	data |= (res - 10) >> 1;
	ret = ad2s1210_set_control(me, data);
	return ret;
}

/**
 * @brief 	reads one register of the chip.
 * @param 	me		: pointer to struct ad2s1210
 * @param 	address	: register address to be read
 * @returns	the value of the addressed register
 */
uint8_t ad2s1210_get_reg(struct ad2s1210 *me, uint8_t address)
{
	uint8_t ret = 0;

	ret = ad2s1210_config_read(me, address | AD2S1210_ADDRESS_MASK);
	return ret;
}

/**
 * @brief 	reads one register of the chip.
 * @param 	me		: pointer to struct ad2s1210
 * @param 	address	: register address to be read
 * @param	data	: the value to store in the register
 * @returns	the value of the addressed register
 */
int32_t ad2s1210_set_reg(struct ad2s1210 *me, uint8_t address,
		uint8_t data)
{
	int32_t ret = 0;

	ret = ad2s1210_config_write(me, address | AD2S1210_ADDRESS_MASK);
	if (ret < 0)
		return ret;
	ret = ad2s1210_config_write(me, data & AD2S1210_DATA_MASK);
	return ret;
}


/**
 * @brief	initializes the chip with the values specified in st
 * @param 	me		: pointer to struct ad2s1210
 * @returns 0 on success
 * @returns < 0 error
 */
int32_t ad2s1210_init(struct ad2s1210 *me)
{
	uint8_t data;
	int32_t ret = 0;

	if (me->resolution < 10 || me->resolution > 16) {
		lDebug(Error, "ad2s1210: resolution out of range");
		return -1;
	}

	if (me->fclkin < AD2S1210_MIN_CLKIN || me->fclkin > AD2S1210_MAX_CLKIN) {
		lDebug(Error, "ad2s1210: fclkin out of range");
		return -1;
	}

	if (me->fexcit < AD2S1210_MIN_EXCIT || me->fexcit > AD2S1210_MAX_EXCIT) {
		lDebug(Error, "ad2s1210: excitation frequency out of range");
		return -1;
	}

	data = AD2S1210_DEF_CONTROL & ~(AD2S1210_RESOLUTION_MASK);
	data |= (me->resolution - 10) >> 1;
	ret = ad2s1210_set_control(me, data);
	if (ret < 0)
		return ret;
	ret = ad2s1210_update_frequency_control_word(me);
	if (ret < 0)
		return ret;
	ret = ad2s1210_soft_reset(me);

	ad2s1210_set_reg(me, AD2S1210_REG_LOS_THRD, 50);
	return ret;
}

/**
 * @brief	reads position registers
 * @param 	me 		: struct ad2s1210
 * @returns position register value
 */
uint16_t ad2s1210_read_position(struct ad2s1210 *me)
{
	uint16_t pos = 0;

	/* The position and velocity registers are updated with a high-to-low
	 transition of the SAMPLE signal. This pin must be held low for at least t16 ns
	 to guarantee correct latching of the data. */
	me->gpios.sample(0);

	//No creo que haga falta delay ya que hay varias instrucciones previas
	//a leer el registro
	pos = ad2s1210_config_read_two(me, AD2S1210_REG_POSITION);
	me->gpios.sample(1);

	if (me->reversed)
		pos ^= 0xFFFF >> (16 - me->resolution);

	if (me->hysteresis)
		pos >>= 16 - me->resolution;

	return (uint16_t) pos;
}

/**
 * @brief	reads velocity registers
 * @param 	me 		: struct ad2s1210
 * @returns velocity register value
 */
int16_t ad2s1210_read_velocity(struct ad2s1210 *me)
{
	int16_t vel = 0;

	/* The position and velocity registers are updated with a high-to-low
	 transition of the SAMPLE signal. This pin must be held low for at least t16 ns
	 to guarantee correct latching of the data. */
	me->gpios.sample(0);

	//No creo que haga falta delay ya que hay varias instrucciones previas
	//a leer el registro
	vel = ad2s1210_config_read_two(me, AD2S1210_REG_VELOCITY);
	vel >>= 16 - me->resolution;
//	if (vel & 0x8000) {
//		int16_t negative = (0xffff >> me->resolution) << me->resolution;
//		vel |= negative;
//	}

	me->gpios.sample(1);
	return vel;
}

/**
 * @brief 	reads the fault register since last sample.
 * @param 	me		: pointer to struct ad2s1210
 * @returns	the value of the fault register
 */
uint8_t ad2s1210_get_fault_register(struct ad2s1210 *me)
{
	uint8_t ret = 0;

	ret = ad2s1210_config_read(me, AD2S1210_REG_FAULT);
	return ret;
}

/**
 * @brief	prints a human readable version of the fault register content.
 * @param 	me		: pointer to struct ad2s1210
 * @returns nothing
 */
void ad2s1210_print_fault_register(struct ad2s1210 *me)
{
	uint8_t fr = ad2s1210_get_fault_register(me);

	if (fr & (1 << 0))
		lDebug(Info, "Configuration parity error");
	if (fr & (1 << 1))
		lDebug(Info, "Phase error exceeds phase lock range");
	if (fr & (1 << 2))
		lDebug(Info, "Velocity exceeds maximum tracking rate");
	if (fr & (1 << 3))
		lDebug(Info, "Tracking error exceeds LOT threshold");
	if (fr & (1 << 4))
		lDebug(Info, "Sine/cosine inputs exceed DOS mismatch threshold");
	if (fr & (1 << 5))
		lDebug(Info, "Sine/cosine inputs exceed DOS overrange threshold");
	if (fr & (1 << 6))
		lDebug(Info, "Sine/cosine inputs below LOS threshold");
	if (fr & (1 << 7))
		lDebug(Info, "Sine/cosine inputs clipped");
}

/**
 * @brief	clears the fault register.
 * @param 	me		: pointer to struct ad2s1210
 * @returns nothing
 */
uint8_t ad2s1210_clear_fault_register(struct ad2s1210 *me)
{
	me->gpios.sample(0);
	/* delay (2 * tck + 20) nano seconds */
	udelay(1);
	me->gpios.sample(1);
	udelay(1);

	uint8_t ret = 0;

	ret = ad2s1210_config_read(me, AD2S1210_REG_FAULT);

	udelay(1);
	me->gpios.sample(0);
	udelay(1);
	me->gpios.sample(1);
	return ret;
}


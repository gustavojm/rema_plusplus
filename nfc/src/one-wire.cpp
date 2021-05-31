/**
 *          This is a component implementing the 1-Wire protocol.
 *          Timing                                         :
 *            A: Write 1 Low time (us)                     : 6
 *            B: Write 1 High time (us)                    : 64
 *            C: Write 0 Low time (us)                     : 60
 *            D: Write 0 High time (us)                    : 10
 *            E: Read delay time (us)                      : 3
 *            A: Read Low time (us)                        : 6
 *            F: Read delay time                           : 55
 *            H: Reset low time (us)                       : 480
 *            I: Reset response time (us)                  : 70
 *            J: Reset wait time after reading device presence (us) : 410
 *            Total slot time (us)                         : 100
 *
 *  * Copyright (c) Original implementation: Omar Isa� Pinales Ayala, 2014, all rights reserved.
 *  * Updated and maintained by Erich Styger, 2014-2020
 *  * Web:         https://mcuoneclipse.com
 *  * SourceForge: https://sourceforge.net/projects/mcuoneclipse
 *  * Git:         https://github.com/ErichStyger/McuOnEclipse_PEx
 *  * All rights reserved.
 *  *
 *  * Redistribution and use in source and binary forms, with or without modification,
 *  * are permitted provided that the following conditions are met:
 *  *
 *  * - Redistributions of source code must retain the above copyright notice, this list
 *  *   of conditions and the following disclaimer.
 *  *
 *  * - Redistributions in binary form must reproduce the above copyright notice, this
 *  *   list of conditions and the following disclaimer in the documentation and/or
 *  *   other materials provided with the distribution.
 *  *
 *  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>

#include "board.h"

#include "one-wire.h"
#include "ring_buffer.h"
#include "wait.h"
#include "FreeRTOS.h"
#include "task.h"

/* global search state and information */
static unsigned char ROM_NO[8];
static uint8_t LastDiscrepancy;
static uint8_t LastFamilyDiscrepancy;
static uint8_t LastDeviceFlag;

/* Rom commands */
#define RC_READ_ROM          0x33
#define RC_MATCH_ROM         0x55
#define RC_SKIP_ROM          0xCC
#define RC_SEARCH_COND       0xEC
#define RC_SEARCH            0xF0
#define RC_RELEASE           0xFF


#define DQ_Init               DQ1_Init()
#define DQ_SetLow             Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 3, 0)   	//DQ1_ClrVal()
#define DQ_Low                Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 3, 0) 	//DQ1_SetOutput()
#define DQ_Floating           Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 3, 0) 	//DQ1_SetInput()
#define DQ_StrongPullUp		  DQ1_StrongPullUp()
#define DQ_Read               (Chip_GPIO_GetPinState(LPC_GPIO_PORT, 3, 0)!=0)

void DQ1_Init(void)
{
	Chip_SCU_PinMuxSet( 6, 1, SCU_MODE_FUNC0 | SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_PINIO_FAST);	//GPIO0	P6_1	PIN74	GPIO3[0]  RESET (SHARED)
	Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 3, 0);
	Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 3, 0);
}

void DQ1_StrongPullUp() {
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 3, 0);
	Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 3, 0);
}

#define INPUT_RING_BUFFER_SIZE	16		// Must be a power of 2 as required by ring_buffer.h impl
RINGBUFF_T	input_ring_buff;

uint8_t ring_buffer_buffer[INPUT_RING_BUFFER_SIZE];

static uint8_t read_bit(void)
{
	uint8_t bit;

	taskENTER_CRITICAL_FROM_ISR();
	DQ_Low;
	wait_us(ONE_WIRE_CONFIG_A_READ_LOW_TIME);
	DQ_Floating;
	wait_us(ONE_WIRE_CONFIG_E_BEFORE_READ_DELAY_TIME);
	bit = DQ_Read;
	taskEXIT_CRITICAL_FROM_ISR(0);
	wait_us(ONE_WIRE_CONFIG_F_AFTER_READ_DELAY_TIME);
	return bit;
}

static void write_bit(uint8_t bit)
{
	if (bit & 1) {
		taskENTER_CRITICAL_FROM_ISR();
		DQ_Low;
		wait_us(ONE_WIRE_CONFIG_A_WRITE_1_LOW_TIME);
		DQ_Floating;
		wait_us(ONE_WIRE_CONFIG_B_WRITE_1_HIGH_TIME);
		taskEXIT_CRITICAL_FROM_ISR(0);
	} else { /* zero bit */
		taskENTER_CRITICAL_FROM_ISR();
		DQ_Low;
		wait_us(ONE_WIRE_CONFIG_C_WRITE_0_LOW_TIME);
		DQ_Floating;
		wait_us(ONE_WIRE_CONFIG_D_WRITE_0_HIGH_TIME);
		taskEXIT_CRITICAL_FROM_ISR(0);
	}
}

/**
 * @brief  	sends a reset to the bus
 * @returns error code
 */
uint8_t one_wire_send_reset(void)
{
	uint8_t bit;

	taskENTER_CRITICAL_FROM_ISR();
	DQ_Low;
	wait_us(ONE_WIRE_CONFIG_H_RESET_TIME);
	DQ_Floating;
	wait_us(ONE_WIRE_CONFIG_I_RESET_RESPONSE_TIME);
	bit = DQ_Read;
	taskEXIT_CRITICAL_FROM_ISR(0);
	wait_us(ONE_WIRE_CONFIG_J_RESET_WAIT_TIME);
	if (!bit) { /* a device pulled the data line low: at least one device is present */
		return ERR_OK;
	} else {
		return ERR_BUSOFF; /* no device on the bus? */
	}
}

/**
 * @brief  	sends a single byte
 * @param   data    : the data byte to be sent
 * @returns error code
 */
uint8_t one_wire_send_byte(uint8_t data)
{
	int i;

	for (i = 0; i < 8; i++) {
		write_bit(data & 1); /* send LSB first */
		data >>= 1; /* next bit */
	} /* for */
	return ERR_OK;
}

/**
 * @brief   sends multiple bytes
 * @param   *data   : pointer to the array of bytes
 * @param   count	: number of bytes to be sent
 * @returns error code
 */
uint8_t one_wire_send_bytes(uint8_t *data, uint8_t count)
{
	uint8_t res;

	while (count > 0) {
		res = one_wire_send_byte(*data);
		if (res != ERR_OK) {
			return res; /* failed */
		}
		data++;
		count--;
	}
	return ERR_OK;
}

/**
 * @brief	programs a read operation after the master send all in
 *         	output buffer. Don't use a SendReset while the data is
 *         	coming.
 * @param	counter	: number of bytes to receive from slave
 * @returns error code
 */
uint8_t one_wire_receive(uint8_t counter)
{
	int i;
	uint8_t val, mask;

	while (counter > 0) {
		val = 0;
		mask = 1;
		for (i = 0; i < 8; i++) {
			if (read_bit()) { /* read bits (LSB first) */
				val |= mask;
			}
			mask <<= 1; /* next bit */
		} /* for */
		(void) RingBuffer_Insert(&input_ring_buff ,&val); /* put it into the queue so it can be retrieved by GetBytes() */
		counter--;
	}
	return ERR_OK;
}

/**
 * @brief 	returns the number of elements stored on input buffer that are ready to read.
 * @returns number of elements
 */
uint8_t one_wire_count(void)
{
	return RingBuffer_GetCount(&input_ring_buff);
}

/**
 * @brief	get a single byte from the bus
 * @param   *data	: pointer to were to store the data
 * @returns error code
 */
uint8_t one_wire_get_byte(uint8_t *data)
{
	if (RingBuffer_GetCount(&input_ring_buff) == 0) {
		return ERR_FAILED;
	}
	(void) RingBuffer_Pop(&input_ring_buff, data);
	return ERR_OK;
}

/**
 * @brief 	gets multiple bytes from the bus
 * @param	*data : pointer to where to store the data
 * @param	count : number of bytes
 * @returns error code
 */
uint8_t one_wire_get_bytes(uint8_t *data, uint8_t count)
{
	if (count > RingBuffer_GetCount(&input_ring_buff)) {
		return ERR_FAILED;
	}
	for (; count > 0; count--) {
		(void) RingBuffer_Pop(&input_ring_buff, data);
		data++;
	}
	return ERR_OK;
}

/**
 * @brief	calculates the CRC over a number of bytes
 * @param   *data 	 : pointer to data
 * @param   dataSize : number of data bytes
 * @returns calculated CRC
 */
uint8_t one_wire_calc_CRC(uint8_t *data, uint8_t dataSize)
{
	uint8_t crc, i, x, y;

	crc = 0;
	for (x = 0; x < dataSize; x++) {
		y = data[x];
		for (i = 0; i < 8; i++) { /* go through all bits of the data byte */
			if ((crc & 0x01) ^ (y & 0x01)) {
				crc >>= 1;
				crc ^= 0x8c;
			} else {
				crc >>= 1;
			}
			y >>= 1;
		}
	}
	return crc;
}

/**
 * @brief	initializes this device.
 * @returns nothing
 */
void one_wire_init(void)
{
	DQ1_Init();
	RingBuffer_Init(&input_ring_buff, ring_buffer_buffer, sizeof(uint8_t), INPUT_RING_BUFFER_SIZE );

	DQ_Floating; /* input mode, let the pull-up take the signal high */
	/* load LOW to output register. We won't change that value afterwards, we only switch between output and input/float mode */
	DQ_SetLow;
}

/**
 * @brief	read the ROM code. Only works with one device on the bus.
 * @param 	*romCodeBuffer   - Pointer to a buffer with 8 bytes where the ROM code gets stored
 * @returns error code
 */
uint8_t one_wire_read_rom_code(uint8_t *romCodeBuffer)
{
	uint8_t res;

	one_wire_send_reset();
	one_wire_send_byte(RC_READ_ROM);
	one_wire_receive(ONE_WIRE_ROM_CODE_SIZE); /* 8 bytes for the ROM code */
	one_wire_send_byte(RC_RELEASE);
	/* copy ROM code */
	res = one_wire_get_bytes(romCodeBuffer, ONE_WIRE_ROM_CODE_SIZE); /* 8 bytes */
	if (res != ERR_OK) {
		return res; /* error */
	}
	/* index 0  : family code
	 index 1-6: 48bit serial number
	 index 7  : CRC
	 */
	if (one_wire_calc_CRC(&romCodeBuffer[0], ONE_WIRE_ROM_CODE_SIZE - 1)
			!= romCodeBuffer[ONE_WIRE_ROM_CODE_SIZE - 1]) {
		return ERR_CRC; /* wrong CRC? */
	}
	return ERR_OK; /* ok */
}

/**
 * @brief 	adds a single character to a zero byte terminated string
 *         	buffer. It cares about buffer overflow.
 * @param   *dst	: pointer to destination string
 * @param 	dstSize : size of the destination buffer (in bytes).
 * @param	ch      : character to append
 * @returns nothing
 */
void chcat(uint8_t *dst, size_t dstSize, uint8_t ch)
{
	dstSize--; /* for zero byte */
	/* point to the end of the source */
	while (dstSize > 0 && *dst != '\0') {
		dst++;
		dstSize--;
	}
	/* copy the ch in the destination */
	if (dstSize > 0) {
		*dst++ = ch;
	}
	/* terminate the string */
	*dst = '\0';
}

/**
 * @brief 	appends a 8bit unsigned value to a string buffer as hex
 *        	number (without a 0x prefix).
 * @param 	*dst    : pointer to destination string
 * @param	dstSize : size of the destination buffer (in bytes).
 * @param   num     : value to convert.
 * @returns nothing
 */
void strcat_num_8_hex(uint8_t *dst, size_t dstSize, uint8_t num)
{
	unsigned char buf[sizeof("FF")]; /* maximum buffer size we need */
	unsigned char hex;

	buf[2] = '\0';
	hex = (char) (num & 0x0F);
	buf[1] = (char) (hex + ((hex <= 9) ? '0' : ('A' - 10)));
	hex = (char) ((num >> 4) & 0x0F);
	buf[0] = (char) (hex + ((hex <= 9) ? '0' : ('A' - 10)));
	strncat((char *) dst, (char *) buf, dstSize);
}

/**
 * @brief   appends the ROM code to a string.
 * @param	*buf	 : pointer to zero terminated buffer
 * @param 	bufSize  : size of buffer
 * @param 	*romCode : pointer to 8 bytes of ROM Code
 * @returns error code
 */
uint8_t one_wire_strcat_rom_code(uint8_t *buf, size_t bufSize, uint8_t *romCode)
{
	int j;

	for (j = 0; j < ONE_WIRE_ROM_CODE_SIZE; j++) {
		strcat_num_8_hex(buf, bufSize, romCode[j]);
		if (j < ONE_WIRE_ROM_CODE_SIZE - 1) {
			chcat(buf, bufSize, '-');
		}
	}
	return ERR_OK;
}

/**
 * @brief   reset the search state
 * @returns nothing
 */
void one_wire_reset_search(void)
{
	/* reset the search state */
	int i;

	LastDiscrepancy = 0;
	LastDeviceFlag = FALSE;
	LastFamilyDiscrepancy = 0;
	for (i = 7;; i--) {
		ROM_NO[i] = 0;
		if (i == 0) {
			break;
		}
	}
}

/**
 * @param   familyCode	: family code to restrict search for
 * @returns nothing
 */
void one_wire_target_search(uint8_t familyCode)
{
	/* set the search state to find SearchFamily type devices */
	int i;

	ROM_NO[0] = familyCode;
	for (i = 1; i < 8; i++) {
		ROM_NO[i] = 0;
	}
	LastDiscrepancy = 64;
	LastFamilyDiscrepancy = 0;
	LastDeviceFlag = FALSE;
}

/**
 * @brief	perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
 * 			search state.
 * @param   *newAddr	: pointer to 8 bytes of data where to store the new address
 * @param   search_mode :
 * @returns TRUE if new device has been found, FALSE otherwise.
 * @note 	version from https://raw.githubusercontent.com/PaulStoffregen/OneWire/master/OneWire.cpp
 */
bool one_wire_search(uint8_t *newAddr, bool search_mode)
{
	uint8_t id_bit_number;
	uint8_t last_zero, rom_byte_number, search_result;
	uint8_t id_bit, cmp_id_bit;
	unsigned char rom_byte_mask, search_direction;
	uint8_t res;

	/* initialize for search */
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	search_result = 0;

	/* if the last call was not the last one */
	if (!LastDeviceFlag) {
		/* 1-Wire reset */
		res = one_wire_send_reset();
		if (res != ERR_OK) {
			/* reset the search */
			LastDiscrepancy = 0;
			LastDeviceFlag = FALSE;
			LastFamilyDiscrepancy = 0;
			return FALSE;
		}
		/* issue the search command */
		if (search_mode) {
			one_wire_send_byte(RC_SEARCH); /* NORMAL SEARCH */
		} else {
			one_wire_send_byte(RC_SEARCH_COND); /* CONDITIONAL SEARCH */
		}
		/* loop to do the search */
		do {
			/* read a bit and its complement */
			id_bit = read_bit();
			cmp_id_bit = read_bit();

			/* check for no devices on 1-wire */
			if ((id_bit == 1) && (cmp_id_bit == 1)) {
				break;
			} else {
				/* all devices coupled have 0 or 1 */
				if (id_bit != cmp_id_bit) {
					search_direction = id_bit; /* bit write value for search */
				} else {
					/* if this discrepancy if before the Last Discrepancy */
					/* on a previous next then pick the same as last time */
					if (id_bit_number < LastDiscrepancy) {
						search_direction = ((ROM_NO[rom_byte_number]
								& rom_byte_mask) > 0);
					} else {
						/* if equal to last pick 1, if not then pick 0 */
						search_direction = (id_bit_number == LastDiscrepancy);
					}
					/* if 0 was picked then record its position in LastZero */
					if (search_direction == 0) {
						last_zero = id_bit_number;
						/* check for Last discrepancy in family */
						if (last_zero < 9)
							LastFamilyDiscrepancy = last_zero;
					}
				}

				/* set or clear the bit in the ROM byte rom_byte_number */
				/* with mask rom_byte_mask */
				if (search_direction == 1) {
					ROM_NO[rom_byte_number] |= rom_byte_mask;
				} else {
					ROM_NO[rom_byte_number] &= ~rom_byte_mask;
				}
				/* serial number search direction write bit */
				write_bit(search_direction);

				/* increment the byte counter id_bit_number */
				/* and shift the mask rom_byte_mask */
				id_bit_number++;
				rom_byte_mask <<= 1;

				/* if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask */
				if (rom_byte_mask == 0) {
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		} while (rom_byte_number < 8); /* loop until through all ROM bytes 0-7 */
		/* if the search was successful then */
		if (!(id_bit_number < 65)) {
			/* search successful so set LastDiscrepancy,LastDeviceFlag,search_result */
			LastDiscrepancy = last_zero;

			/* check for last device */
			if (LastDiscrepancy == 0) {
				LastDeviceFlag = TRUE;
			}
			search_result = TRUE;
		}
	}
	/* if no device found then reset counters so next 'search' will be like a first */
	if (!search_result || !ROM_NO[0]) {
		LastDiscrepancy = 0;
		LastDeviceFlag = FALSE;
		LastFamilyDiscrepancy = 0;
		search_result = FALSE;
	} else {
		int i;

		for (i = 0; i < 8; i++) {
			newAddr[i] = ROM_NO[i];
		}
	}
	return search_result;
}

void one_wire_strong_pull_up(uint32_t ms) {
	DQ1_StrongPullUp();
	vTaskDelay(pdMS_TO_TICKS(ms));
	DQ1_Init();
}

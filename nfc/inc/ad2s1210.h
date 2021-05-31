#ifndef AD2S1210_H_
#define AD2S1210_H_

#include <stdint.h>
#include <stdbool.h>

#include "poncho_rdc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The default value of the control register on power-up */
#define AD2S1210_DEF_CONTROL			0x7E

#define AD2S1210_ADDRESS_MASK			1 << 7
#define AD2S1210_DATA_MASK				~(1 << 7)

/* Control register bits description */
#define AD2S1210_PHASE_LOCK_RANGE_44	1 << 5		///< 0=360º, 1=44º
#define AD2S1210_HYSTERESIS				1 << 4		///< 0=disabled, 1=enabled
#define AD2S1210_ENCODER_RES1			1 << 3		///< Encoder output resolution (not used)
#define AD2S1210_ENCODER_RES0			1 << 2		///< Encoder output resolution (not used)
#define AD2S1210_RES1					1 << 1      ///< Pos and Vel resolution
#define AD2S1210_RES0					1 << 0		///< Pos and Vel resolution

#define AD2S1210_RESOLUTION_MASK		(AD2S1210_RES1 | AD2S1210_RES0)

#define AD2S1210_MSB_MASK				1 << 7

#define AD2S1210_REG_POSITION			0x80
#define AD2S1210_REG_VELOCITY			0x82

/* Loss of Signal (LOS) register address */
#define AD2S1210_REG_LOS_THRD			0x88

/* Degradation of Signal (DOS) register addresses */
#define AD2S1210_REG_DOS_OVR_THRD		0x89
#define AD2S1210_REG_DOS_MIS_THRD		0x8A
#define AD2S1210_REG_DOS_RST_MAX_THRD	0x8B
#define AD2S1210_REG_DOS_RST_MIN_THRD	0x8C

/* Loss of Tracking (LOT) register addresses */
#define AD2S1210_REG_LOT_HIGH_THRD		0x8D
#define AD2S1210_REG_LOT_LOW_THRD		0x8E

/* Excitation Frequency (EXCIT) register address */
#define AD2S1210_REG_EXCIT_FREQ			0x91

#define AD2S1210_REG_CONTROL			0x92
#define AD2S1210_REG_SOFT_RESET			0xF0
#define AD2S1210_REG_FAULT				0xFF

#define AD2S1210_MIN_CLKIN	6144000
#define AD2S1210_MAX_CLKIN	10240000
#define AD2S1210_MIN_EXCIT	2000
#define AD2S1210_MAX_EXCIT	20000
#define AD2S1210_MIN_FCW	0x4
#define AD2S1210_MAX_FCW	0x50

#define AD2S1210_DEF_EXCIT	10000

/**
 * @struct 	ad2s1210_gpios
 * @brief	pointers to functions to handle GPIO lines of AD2S121.
 */
struct ad2s1210_gpios {
	void (*reset)(bool);		///< pointer to RESET line function handler
	void (*sample)(bool);		///< pointer to SAMPLE line function handler
	void (*wr_fsync)(bool);		///< pointer to WR_FSYNC line function handler
};

/**
 * @struct 	ad2s1210
 * @brief	RDC device instance specific state.
 */
struct ad2s1210 {
	struct ad2s1210_gpios gpios;///< specific gpio function pointers for this instance
	uint32_t fclkin;			///< frequency of clock input
	uint32_t fexcit;			///< excitation frequency
	bool hysteresis;			///< cache of whether hysteresis is enabled
	bool reversed;				///< if set will reverse the position readings
	uint8_t resolution;			///< chip resolution could be 10/12/14/16-bit
};

int32_t ad2s1210_init(struct ad2s1210 *me);

int32_t ad2s1210_soft_reset(struct ad2s1210 *me);

void ad2s1210_hard_reset(struct ad2s1210 *me);

uint32_t ad2s1210_get_fclkin(struct ad2s1210 *me);

int32_t ad2s1210_set_fclkin(struct ad2s1210 *me, uint32_t fclkin);

uint32_t ad2s1210_get_fexcit(struct ad2s1210 *me);

int32_t ad2s1210_set_fexcit(struct ad2s1210 *me, uint32_t fexcit);

uint8_t ad2s1210_get_resolution(struct ad2s1210 *me);

int32_t ad2s1210_set_resolution(struct ad2s1210 *me, uint8_t res);

uint8_t ad2s1210_get_reg(struct ad2s1210 *me, uint8_t address);

int32_t ad2s1210_set_reg(struct ad2s1210 *me, uint8_t address,
		uint8_t data);

uint16_t ad2s1210_read_position(struct ad2s1210 *me);

int16_t ad2s1210_read_velocity(struct ad2s1210 *me);

uint8_t ad2s1210_get_fault_register(struct ad2s1210 *me);

void ad2s1210_print_fault_register(struct ad2s1210 *me);

uint8_t ad2s1210_clear_fault_register(struct ad2s1210 *me);

#ifdef __cplusplus
}
#endif

#endif /* AD2S1210_H_ */

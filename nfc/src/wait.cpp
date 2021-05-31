#include "wait.h"

#include <stdint.h>
#include <stdbool.h>

#include "board.h"

/**
 * @brief 	Wait for 10 cpu cycles.
 * @returns	nothing
 */
__attribute__((naked, no_instrument_function))
void wait_10cycles(void)
{
	/* This function will wait 10 CPU cycles (including call overhead). */
	/* NOTE: Cortex-M0 and M4 have 1 cycle for a NOP */
	__asm (
			/* bl Wait10Cycles() to here: [4] */
			"nop   \n\t" /* [1] */
			"nop   \n\t" /* [1] */
			"nop   \n\t" /* [1] */
			"bx lr \n\t" /* [3] */
	);
}

/**
 * @brief 	Wait for a specified number of CPU cycles.
 * @param	cycles	:the number of cycles to wait
 * @returns	nothing
 */
void wait_cycles(uint32_t cycles)
{
	uint32_t counter = cycles;

	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	counter += DWT->CYCCNT;
	while (DWT->CYCCNT < counter) {
		/* wait */
		__asm("");
	}
}

/**
 * @brief 	Wait for a specified time in milliseconds.
 * @param	ms	:How many milliseconds the function has to wait
 * @returns	nothing
 */
void wait_ms(uint32_t ms)
{
	uint32_t msCycles; /* cycles for 1 ms */

	/* static clock/speed configuration */
	msCycles = number_of_cycles_ms(1, INSTR_CLOCK_HZ);
	while (ms > 0) {
		wait_cycles(msCycles);
		ms--;
	}
}


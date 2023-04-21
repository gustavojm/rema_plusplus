#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "tmr.h"
#include "debug.h"
#include "mot_pap.h"

#define TMR_INTERRUPT_PRIORITY  (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2)

/**
 * @brief	enables timer 0(Pole)/1(Arm) clock and resets it
 * @returns	nothing
 */
tmr::tmr(LPC_TIMER_T *lpc_timer, CHIP_RGU_RST_T rgu_timer_rst,
        CHIP_CCU_CLK_T clk_mx_timer,
        IRQn_Type timer_IRQn) :
        lpc_timer(lpc_timer),             // Initializer list
        rgu_timer_rst(rgu_timer_rst),
                clk_mx_timer(clk_mx_timer),
                timer_IRQn(timer_IRQn) {
    started = false;

    Chip_TIMER_Init(lpc_timer);
    Chip_RGU_TriggerReset(rgu_timer_rst);

    while (Chip_RGU_InReset(rgu_timer_rst)) {
    }

    Chip_TIMER_Reset(lpc_timer);
    Chip_TIMER_MatchEnableInt(lpc_timer, 1);
    Chip_TIMER_ResetOnMatchEnable(lpc_timer, 1);
}

/**
 * @brief	sets TIMER 0(Pole)/1(Arm) frequency
 * @param 	tick_rate_hz 	: desired frequency
 * @returns	0 on success
 * @returns	-1 if tick_rate_hz > 300000
 *
 */
int32_t tmr::set_freq(uint32_t tick_rate_hz) {
    uint32_t timerFreq;

    Chip_TIMER_Reset(lpc_timer);

    if ((tick_rate_hz < 0) || (tick_rate_hz > MOT_PAP_COMPUMOTOR_MAX_FREQ)) {
        //lDebug(Error, "Timer: invalid freq");
        return -1;
    }

    /* Get timer 0 peripheral clock rate */
    timerFreq = Chip_Clock_GetRate(clk_mx_timer);

    tick_rate_hz = tick_rate_hz << 1;           // Double the frequency
    /* Timer setup for match at tick_rate_hz */
    Chip_TIMER_SetMatch(lpc_timer, 1, (timerFreq / tick_rate_hz));
    return 0;
}

void tmr::change_freq(uint32_t tick_rate_hz) {
    stop();
    set_freq(tick_rate_hz);
    start();
}


/**
 * @brief 	enables timer interrupt and starts it
 * @returns	nothing
 */
void tmr::start() {
    NVIC_ClearPendingIRQ(timer_IRQn);
    Chip_TIMER_Enable(lpc_timer);
    NVIC_SetPriority(timer_IRQn, TMR_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(timer_IRQn);
    started = true;
}

/**
 * @brief 	disables timer interrupt and stops it
 * @returns	nothing
 */
void tmr::stop() {
    Chip_TIMER_Disable(lpc_timer);
    NVIC_DisableIRQ(timer_IRQn);
    NVIC_ClearPendingIRQ(timer_IRQn);
    Chip_TIMER_Reset(lpc_timer);
    started = false;
}

/**
 * @brief	returns if timer is started by querying if the interrupt is enabled
 * @returns 0 timer is not started.
 * @returns 1 timer is started.
 */
uint32_t tmr::is_started() {
    return started;
}

/**
 * @brief	Determine if a match interrupt is pending
 * @returns 0 timer is not started.
 * @returns false if the interrupt is not pending, otherwise true
 * @note	Determine if the match interrupt for the passed timer and match
 * 			counter is pending. If the interrupt is pending clears the match counter
 */
bool tmr::match_pending() {
    bool ret = Chip_TIMER_MatchPending(lpc_timer, 1);
    if (ret) {
        Chip_TIMER_ClearMatch(lpc_timer, 1);
    }
    return ret;
}

#ifndef WAIT_H_
#define WAIT_H_

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

#include "lift.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INSTR_CLOCK_HZ       SystemCoreClock  /* core clock frequency in Hz */

#define number_of_cycles_ms(ms, hz)  ((ms)*((hz)/1000)) /* calculates the needed cycles based on bus clock frequency */
#define number_of_cycles_us(us, hz)  ((us)*(((hz)/1000)/1000)) /* calculates the needed cycles based on bus clock frequency */
#define number_of_cycles_ns(ns, hz)  (((ns)*(((hz)/1000)/1000))/1000) /* calculates the needed cycles based on bus clock frequency */

#define WAIT_C(cycles) \
     ( (cycles)<=10 ? \
          wait_10cycles() \
        : wait_cycles(cycles) \
      )                                      /*!< wait for some cycles */

#define wait_us(us)  \
       (  ((number_of_cycles_us((us),INSTR_CLOCK_HZ)==0)||(us)==0) ? \
          (void)0 : \
          ( ((us)/1000)==0 ? (void)0 : wait_ms(((us)/1000))) \
          , (number_of_cycles_us(((us)%1000), INSTR_CLOCK_HZ)==0) ? (void)0 : \
            WAIT_C(number_of_cycles_us(((us)%1000), INSTR_CLOCK_HZ)) \
       )

#define wait_ns(ns)  \
       (  ((number_of_cycles_ns((ns), INSTR_CLOCK_HZ)==0)||(ns)==0) ? \
          (void)0 : \
          wait_us((ns)/1000) \
          , (number_of_cycles_ns((ns)%1000, INSTR_CLOCK_HZ)==0) ? \
              (void)0 : \
              WAIT_C(number_of_cycles_ns(((ns)%1000), INSTR_CLOCK_HZ)) \
       )

__attribute__((naked, no_instrument_function))
void wait_10cycles(void);

void wait_cycles(uint32_t cycles);

void wait_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* WAIT_H_ */

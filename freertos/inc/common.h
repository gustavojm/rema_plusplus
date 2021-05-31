/*
  * Copyright 2018 Krzysztof Hockuba
  *
  * Permission is hereby granted, free of charge, to any person obtaining
  * a copy of this software and associated documentation files (the "Software"),
  * to deal in the Software without restriction, including without limitation
  * the rights to use, copy, modify, merge, publish, distribute, sublicense,
  * and/or sell copies of the Software, and to permit persons to whom the
  * Software is furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included
  *          in all copies or substantial portions of the Software.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>

#include "board.h" // Include apropriate header for used mcu
#include "FreeRTOS.h"
#include "task.h"
extern uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];

#define OPTIMIZE_FAST __attribute__((optimize("O3")))
#define OPTIMIZE_SIZE __attribute__((optimize("Os")))
#define OPTIMIZE_NONE __attribute__((optimize("O0")))
#define PUT_IN_RAM2   __attribute__((section(".ramSection")))

#define DBG_ENABLE_MEM_FAULT() SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk
#define DBG_ENABLE_BUS_FAULT() SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk
#define DBG_ENABLE_USAGE_FAULT() SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk

#define BIT_SET(a,b) 		((a) |= (1<<b))
#define BIT_CLEAR(a,b) 		((a) &= ~(1<<b))
#define BIT_FLIP(a,b) 		((a) ^= (1<<b))
#define BIT_CHECK(a,b) 		((a) & (1<<b))

#define BITMASK_SET(a,b) 	((a) |= (b))
#define BITMASK_CLEAR(a,b) 	((a) &= ~(b))
#define BITMASK_FLIP(a,b) 	((a) ^= (b))
#define BITMASK_CHECK(a,b) 	((a) & (b))

#define BYTE_TO_I16(x, y) 	((int16_t)x[0] | (int16_t) x[1] << 8)
#define BYTE_TO_U16(x, y) 	((uint16_t)x[0] | (uint16_t) x[1] << 8)
#define BYTE_TO_I32(x, y) 	((int16_t)x[0] | (int16_t) x[1] << 8 | (int16_t) x[2] << 16 | (int16_t) x[3] << 24)
#define BYTE_TO_U32(x, y) 	((uint32_t)x[0] | (uint32_t) x[1] << 8 | (uint32_t) x[2] << 16 | (uint32_t) x[3] << 24)

#define UI16_TO_BYTE(x, y)  (x)[0] = (uint8_t)(y & 0xFF); (x)[1] = (uint8_t)((y >> 8) & 0xFF)
#define UI32_TO_BYTE(x, y)  (x)[0] = (uint8_t)(y & 0xFF); (x)[1] = (uint8_t)((y >> 8) & 0xFF); (x)[2] = (uint8_t)((y >> 16) & 0xFF); (x)[3] = (uint8_t)((y >> 24) & 0xFF);

#ifndef UNIT_TEST
/** Define base address of bit-band **/
#define BITBAND_SRAM_BASE 0x20000000
/** Define base address of alias band **/
#define ALIAS_SRAM_BASE 0x22000000
/** Convert SRAM address to alias region **/
#define BITBAND_SRAM(a, b) ((ALIAS_SRAM_BASE + ((uint32_t) & (a) - BITBAND_SRAM_BASE ) * 32 + (b * 4)))

/** Define base address of peripheral bit-band **/
#define BITBAND_PERI_BASE 0x40000000
/** Define base address of peripheral alias band **/
#define ALIAS_PERI_BASE 0x42000000
/** Convert PERI address to alias region **/
#define BITBAND_PERI(a, b) (uint32_t*)((ALIAS_PERI_BASE + ((uint32_t) a - BITBAND_PERI_BASE) * 32 + (b * 4)))
/** Used to offset Register addtess **/
#define REG_OFFSET(a, b) (a + b)
/** Check if we are in debug session **/
#define IS_DEBUGGER_CONNECTED() READ_BIT(CoreDebug->DHCSR, CoreDebug_DHCSR_C_DEBUGEN_Msk)

/** Check if address x is allocated **/
#ifdef RTOS

static inline uint8_t isCurrentTaskStackAddress(uint32_t *x) {
    TaskStatus_t status;
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    vTaskGetInfo(current_task, &status, 0, 0);
    uint32_t *startHeap = status.pxStackBase;
    uint32_t *endHeap = current_task;
    if (x > startHeap && x < endHeap) {
        return 1;
    }
    return 0;
}

#define IS_MALLOC_ADDRES(x) ((((uint8_t*)(x) > ucHeap) && ((uint8_t*)(x) < (ucHeap + configTOTAL_HEAP_SIZE)) && (isCurrentTaskStackAddress((uint32_t*)(x)) == 0)) ? 1 : 0)
#else
#define IS_MALLOC_ADDRES(x) ((((uint32_t)x & 0xF0000000) == 0x20000000) ? 1 : 0)
#endif

/**
  \brief   Get IPSR Register
  \details Returns the content of the IPSR Register.
  \return  IPSR Register value
 */
__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __get_LR(void)
{
  uint32_t result;

  __ASM volatile ("MOV %0, lr" : "=r" (result) );
  return(result);
}

/**
  \brief   Get PC Register
  \details Returns the content of the PC Register.
  \return  PC Register value
 */
__attribute__( ( always_inline ) ) __STATIC_INLINE uint32_t __get_PC(void)
{
  uint32_t result;

  __ASM volatile ("MOV %0, pc" : "=r" (result) );
  return(result);
}

#endif

#endif

#ifndef	CUSTOM_ALLOC_H_
#define	CUSTOM_ALLOC_H_

#include "FreeRTOS.h"

// https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
#if	configUSE_MALLOC_DEBUG == 1
#define pvPortMalloc_debug(size) ({ \
	void *__ptr__ = pvPortMalloc(size); \
	printf("[ALLOC] %32s:%4d | addr= %p, size= %lu, expr= `%s`\n" \
		, __FUNCTION__, __LINE__ \
		, __ptr__, size \
		, #size); \
	__ptr__; \
})
#else
#define pvPortMalloc_debug(size)		pvPortMalloc(size)
#endif

#if	configUSE_MALLOC_DEBUG == 1
#define vPortFree_debug(ptr) ({ \
	printf("[ FREE] %32s:%4d | addr= %p, expr= `%s`\n" \
		, __FUNCTION__, __LINE__ \
		, ptr\
		, #ptr); \
	vPortFree(ptr); \
})

#else
#define vPortFree_debug(size)		vPortFree(size)
#endif

#endif	/*  CUSTOM_ALLOC_H_  */

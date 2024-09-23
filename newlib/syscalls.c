#include <errno.h>
#include <malloc.h>
#include <signal.h>
#include <sys/reent.h>

#include "FreeRTOS.h"
#include "task.h"

#define WEAK      __attribute__((__used__, weak))
#define WEAK_INIT __attribute__((__used__, weak, constructor))
#define USED      __attribute__((__used__))

#define UNUSED(x) (void)x

#ifndef USE_SEMIHOST

USED void _exit(int code) {
    UNUSED(code);
}

USED int _close(int fd) {
    UNUSED(fd);
    return -1;
}

USED int _isatty(int fd) {
    switch (fd) {
    case 0:
    case 1:
    case 2: return 1;
    default: return -1;
    }
}

USED int _kill(int pid, int signal) {
    UNUSED(pid);
    UNUSED(signal);
    return -1;
}

USED _off_t _lseek(int fd, _off_t off, int w) {
    UNUSED(fd);
    UNUSED(off);
    UNUSED(w);
    return -1;
}

USED int _getpid() {
    return -1;
}

struct stat { };

USED int _fstat(int fd, struct stat *st) {
    UNUSED(fd);
    UNUSED(st);
    return -1;
}

USED int _open(const char *name, int f, int m) {
    UNUSED(name);
    UNUSED(f);
    UNUSED(m);
    return -1;
}

#endif

//      These values come from the linker file.
//
extern uint8_t _pvHeapStart; // located at the first byte of heap
extern uint8_t _pvHeapEnd;   // located at the first byte after the heap

static uint8_t *currentHeapPos = &_pvHeapStart;

// newlib_malloc_helpers

//      The application may choose to override this function.  Note that
// after this function runs, the malloc-failed hook in FreeRTOS is likely
// to be called too.  It won't be called, however, if we're here due to a
// "direct" call to malloc(), not via pvPortMalloc(), such as a call made
// from inside newlib.
//
WEAK void sbrkFailedHook(ptrdiff_t size) {
    configASSERT(0);
}

void *_sbrk(ptrdiff_t size) {
    // Chip_UART_SendBlocking(DEBUG_UART, "S\n", 2);       // unable to use print
    // here... will recurse
    void *returnValue;

    if (currentHeapPos + size > &_pvHeapEnd) {
        sbrkFailedHook(size);
        returnValue = (void *)-1;
    } else {
        returnValue = (void *)currentHeapPos;
        currentHeapPos += size;
    }

    return (returnValue);
}

//      The implementation of _sbrk_r() included in some builds of newlib
// doesn't enforce a limit in heap size.  This implementation does.
//
void *_sbrk_r(struct _reent *reent, ptrdiff_t size) {
    // Chip_UART_SendBlocking(DEBUG_UART, "SR\n", 3);    // unable to use print
    // here... will recurse
    void *returnValue;

    if (currentHeapPos + size > &_pvHeapEnd) {
        sbrkFailedHook(size);

        reent->_errno = ENOMEM;
        returnValue = (void *)-1;
    } else {
        returnValue = (void *)currentHeapPos;
        currentHeapPos += size;
    }

    return (returnValue);
}

// In the pre-emptive multitasking environment provided by FreeRTOS,
// it's possible (though unlikely) that a call from newlib code to the
// malloc family might be preempted by a higher priority task that then
// makes a call to the malloc family.  These implementations of newlib's
// hook functions protect against that.  Most calls to this function come
// via pvPortMalloc(), which has already suspended the scheduler, so the
// call here is redundant most of the time.  That's OK.
//
void __malloc_lock(struct _reent *r) {
    // Chip_UART_SendBlocking(DEBUG_UART, "ML\n", 3);      // unable to use print
    // here... will recurse
    vTaskSuspendAll();
}

void __malloc_unlock(struct _reent *r) {
    // Chip_UART_SendBlocking(DEBUG_UART, "MU\n", 3);      // unable to use print
    // here... will recurse
    xTaskResumeAll();
}

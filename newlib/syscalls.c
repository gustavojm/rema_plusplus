#include <errno.h>
#include <signal.h>
#include <sys/reent.h>

#define WEAK __attribute__ ((__used__, weak))
#define WEAK_INIT __attribute__ ((__used__, weak, constructor))
#define USED __attribute__ ((__used__))

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
   case 2:
       return 1;
   default:
       return -1;
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

void *_sbrk ( uint32_t delta ) {
extern char _pvHeapStart;             /* Defined by the linker */
static char *heap_end;
char *prev_heap_end;
   if (heap_end == 0) {
    heap_end = &_pvHeapStart;
  }
  prev_heap_end = heap_end;
  // if (prev_heap_end+delta > get_stack_pointer()) {
  //       return (void *) -1L;
  // }
  heap_end += delta;
  return (void *) prev_heap_end;
}
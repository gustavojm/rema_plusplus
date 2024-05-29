#ifndef DEBUG_H_
#define DEBUG_H_

/**
 * @file debug.h
 * @brief Define a <b>simple</b> debugging interface to use in C programs
 * @details One method of debugging your C program is to add <tt>printf()</tt>
 * statements to your code. This file provides a way of including debug
 * output, and being able to turn it on/off either at compile time or
 * at runtime, without making <b>any</b> changes to your code.
 * Two levels of debugging are provided. If the value <tt>DEBUG</tt> is
 * defined, your debug calls are compiled into your code. Otherwise, they are
 * removed by the optimizer. There is an additional run time check as to
 * whether to actually print the debugging output. This is controlled by the
 * value <tt>debugLevel</tt>.
 * <p>
 * To use it with <tt>gcc</tt>, simply write a <tt>printf()</tt> call, but
 * replace the <tt>printf()</tt> call by <tt>debug()</tt>.
 * <p>
 * To easily print the value of a <b>single</b> variable, use
 * <pre><tt>
 vDebug("format", var); // "format" is the specifier (e.g. "%d" or "%s", etc)
 * </tt></pre>
 * <p>
 * To use debug(), but control when it prints, use
 * <pre><tt>
 lDebug(level, "format", var); // print when debugLevel >= level
 * </tt></pre>
 * <p>
 * Based on code and ideas found
 * <a
 href="http://stackoverflow.com/questions/1644868/c-define-macro-for-debug-printing">
 * here</a> and
 * <a
 href="http://stackoverflow.com/questions/679979/how-to-make-a-variadic-macro-variable-number-of-arguments">
 * here</a>.
 * <p>
 * @author Fritz Sieker
 */

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

enum debugLevels {
  Debug,
  Info,
  InfoLocal,
  Warn,
  Error,
};

static inline const char *levelText(enum debugLevels level) {
  const char *ret;
  switch (level) {
  case Debug:
    ret = "Debug";
    break;
  case Info:
  case InfoLocal:
    ret = "Info";
    break;
  case Warn:
    ret = "Warn";
    break;
  case Error:
    ret = "Error";
    break;
  default:
    ret = "";
    break;
  }
  return ret;
}

/**
 * controls how much debug output is produced. Higher values produce more
 * output. See the use in <tt>lDebug()</tt>.
 */
extern enum debugLevels debugLocalLevel;
extern enum debugLevels debugNetLevel;
extern SemaphoreHandle_t uart_mutex;
extern QueueHandle_t debug_queue;
extern bool debug_to_uart;
extern bool debug_to_network;

/**
 * The file where debug output is written. Defaults to <tt>stderr</tt>.
 * <tt>debugToFile()</tt> allows output to any file.
 */
extern FILE *debugFile;

void debugInit();

void debugLocalSetLevel(bool enable, enum debugLevels lvl);

void debugNetSetLevel(bool enable, enum debugLevels lvl);

void debugToFile(const char *fileName);

void debugClose();

/** Prints the file name, line number, function name and "HERE" */
#define HERE debug("HERE")

/**
 * Expands a name into a string and a value.
 * @param name name of variable
 */
#define debugV(name) #name, (name)

/**
 * @brief     outputs the name and value of a single variable.
 * @param     fmt     : format to print the var
 * @param     name     : name of the variable to print
 */
#define vDebug(fmt, name) debug("%s=(" fmt ")", debugV(name))

/** Simple alias for <tt>lDebug()</tt> */
#define debug(fmt, ...) lDebug(1, fmt, ##__VA_ARGS__)

static inline char *make_message(const char *fmt, ...) {
  int size = 0;
  char *p = NULL;
  va_list ap;

  /* Determine required size */

  va_start(ap, fmt);
  size = vsnprintf(p, size, fmt, ap);
  va_end(ap);

  if (size < 0)
    return NULL;

  size++; /* For '\0' */
  p = new char[size];
  if (p == NULL)
    return NULL;

  va_start(ap, fmt);
  size = vsnprintf(p, size, fmt, ap);
  if (size < 0) {
    delete[] p;
    return NULL;
  }
  va_end(ap);

  return p;
}

/**
 * This macro controls whether all debugging code is optimized out of the
 * executable, or is compiled and controlled at runtime by the
 * <tt>debugLevel</tt> variable. Depends on whether
 * the macro <tt>NDEBUG</tt> is defined during the compile.
 */
#ifdef NDEBUG
#define lDebug(level, fmt, ...)
#else
/**
 * @brief prints this message if the variable <tt>debugLevel</tt> is greater
 * than or equal to the parameter.
 * @param level the level at which this information should be printed
 * @param fmt the formatting string (<b>MUST</b> be a literal
 */
#define lDebug(level, fmt, ...)                                                \
  do {                                                                         \
    if (debug_to_uart && debugLocalLevel <= level) {                           \
      if (uart_mutex != NULL) {                                                \
        if (xSemaphoreTake(uart_mutex, portMAX_DELAY) == pdTRUE) {             \
          printf("%lu - %s %s[%d] %s() " fmt "\n", xTaskGetTickCount(),        \
                 levelText(level), __FILE__, __LINE__, __func__,               \
                 ##__VA_ARGS__);                                               \
          xSemaphoreGive(uart_mutex);                                          \
        }                                                                      \
      }                                                                        \
    }                                                                          \
                                                                               \
    if (debug_to_network && debug_queue != nullptr &&                          \
        (debugNetLevel <= level) && (level != InfoLocal)) {                    \
      char *dbg_msg = make_message(                                            \
          "%u - %s %s[%d] %s() " fmt, xTaskGetTickCount(), levelText(level),   \
          __FILE__, __LINE__, __func__, ##__VA_ARGS__);                        \
      if (xQueueSend(debug_queue, &dbg_msg, (TickType_t)0) != pdPASS) {        \
        delete[] dbg_msg;                                                      \
        dbg_msg = NULL;                                                        \
      }                                                                        \
    }                                                                          \
  } while (0)
#endif

#endif

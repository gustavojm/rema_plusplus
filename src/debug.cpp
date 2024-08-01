#include "debug.h"

#include <stdio.h>

void debugInit() {
  uart_mutex = xSemaphoreCreateMutex();
  debug_queue = xQueueCreate(NET_DEBUG_QUEUE_SIZE, sizeof(char *));
}

/**
 * @brief 	sets local debug level.
 * @param 	enable 	:whether or not it will be sent to UART
 * @param 	lvl 	:minimum level to print
 */
void debugLocalSetLevel(bool enable, enum debugLevels lvl) {
  debug_to_uart = enable;
  debugLocalLevel = lvl;
}

/**
 * @brief 	sets debug level to send to network.
 * @param 	enable 	:whether or not it will be sent to network
 * @param 	lvl 	:minimum level to print
 */
void debugNetSetLevel(bool enable, enum debugLevels lvl) {
  debug_to_network = enable;
  debugNetLevel = lvl;
}

/**
 * @brief sends debugging output to a file.
 * @param fileName name of file to send output to
 */
void debugToFile(const char *fileName) {
  debugClose();

  FILE *file = fopen(fileName, "w"); // "w+" ?

  if (file) {
    debugFile = file;
  }
}

/** Close the output file if it was set in <tt>toFile()</tt> */
void debugClose() {
  if (debugFile && (debugFile != stderr)) {
    fclose(debugFile);
    debugFile = stderr;
  }
}

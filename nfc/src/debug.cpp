#include "debug.h"

#include <stdio.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

enum debugLevels debugLocalLevel = Info;
enum debugLevels debugNetLevel = Info;

QueueHandle_t debug_queue = NULL;
bool debug_to_network = true;

FILE *debugFile = NULL;

SemaphoreHandle_t uart_mutex;

void debugInit(void)
{
	uart_mutex = xSemaphoreCreateMutex();
	debug_queue = xQueueCreate(10, sizeof(char *));

}

/**
 * @brief 	sets local debug level.
 * @param 	lvl 	:minimum level to print
 */
void debugLocalSetLevel(enum debugLevels lvl)
{
	debugLocalLevel = lvl;
}

/**
 * @brief 	sets debug level to send to network.
 * @param 	lvl 	:minimum level to print
 */
void debugNetSetLevel(enum debugLevels lvl)
{
	debugNetLevel = lvl;
}

/**
 * @brief sends debugging output to a file.
 * @param fileName name of file to send output to
 */
void debugToFile(const char *fileName)
{
	debugClose();

	FILE *f = fopen(fileName, "w"); // "w+" ?

	if (f)
		debugFile = f;
}

/** Close the output file if it was set in <tt>toFile()</tt> */
void debugClose(void)
{
	if (debugFile && (debugFile != stderr)) {
		fclose(debugFile);
		debugFile = stderr;
	}
}

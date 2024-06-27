/*
 * FreeRTOS Kernel V10.0.1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of http://www.FreeRTOS.org for more information.
 */
#include <stdlib.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"
#include "common.h"


#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
	#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE	( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE		( ( size_t ) 8 )

/* Allocate the memory for the heap. */
#if( configAPPLICATION_ALLOCATED_HEAP == 1 )
	/* The application writer has already defined the array used for the RTOS
	heap - probably so it can be placed in a special segment or address. */
	extern uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#else
	static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif /* configAPPLICATION_ALLOCATED_HEAP */

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
#if( configUSE_MALLOC_DEBUG == 1 )
    uint32_t head_canary;                   /*<< Head Canary, TODO: Remove */
#endif
	struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< The next free block in the list. */
	size_t xBlockSize;						/*<< The size of the free block. */
#if( configUSE_MALLOC_DEBUG == 1 )
    TaskHandle_t *xOwner;                   /*<< Buffer owner, TODO: Remove */
#endif
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert );

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMalloc() is called.
 */
static void prvHeapInit( void );

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize	= ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;


#if( configUSE_MALLOC_DEBUG == 1 )
#define HEAD_CANARY_PATTERN 0xABBA1234
#define TAIL_CANARY_PATTERN 0xBAAD5678

void vPortAddToList(uint32_t pointer);
uint8_t vPortRmFromList(uint32_t pointer);
#endif

BlockLink_t *allocList[256] = {0};


/*************************
 *
 */

#if( configUSE_MALLOC_DEBUG == 1 )
// Additional functions to scan memory (buffer overflow, memory leaks)

#define GET_BUFFER_ADDRESS(x) (uint32_t*)((uint32_t)x + xHeapStructSize)
#define HEAD_CANARY(x) (x->head_canary)
#define TAIL_CANARY(x) *(uint32_t*)((uint32_t)x + (x->xBlockSize & ~xBlockAllocatedBit) - 4)

void OPTIMIZE_FAST vPortCheckIntegrity(void) {
    //Scan free list
    BlockLink_t *start = &xStart;
    do {
        configASSERT(HEAD_CANARY(start) == HEAD_CANARY_PATTERN);
        start = start->pxNextFreeBlock;
    } while (start->pxNextFreeBlock != NULL);

    //Scan allocated list
    uint16_t pos = 0;
    do {
        if (HEAD_CANARY(allocList[pos]) != HEAD_CANARY_PATTERN) {
            printf("Detected buffer overflow\n");
            if (allocList[pos]->xOwner) {
                TaskStatus_t status;
                vTaskGetInfo(allocList[pos]->xOwner, &status, 0, 0);
                uint32_t buffer_address = (uint32_t)allocList[pos] + xHeapStructSize;
                uint32_t buffer_size = (uint32_t)allocList[pos]->xBlockSize & ~xBlockAllocatedBit;
                printf("Task owner %s buffer address %lx size %lu\n", status.pcTaskName, buffer_address, buffer_size);
            }
        }
        configASSERT(HEAD_CANARY(allocList[pos]) == HEAD_CANARY_PATTERN);
        if (TAIL_CANARY(allocList[pos]) != TAIL_CANARY_PATTERN) {
            printf("Detected buffer overflow\n");
            if (allocList[pos]->xOwner) {
                TaskStatus_t status;
                vTaskGetInfo(allocList[pos]->xOwner, &status, 0, 0);
                uint32_t buffer_address = (uint32_t)allocList[pos] + xHeapStructSize;
                uint32_t buffer_size = (uint32_t)allocList[pos]->xBlockSize & ~xBlockAllocatedBit;
                printf("Task owner %s buffer address %lx size %lu\n", status.pcTaskName, buffer_address, buffer_size);
            }
        }
        configASSERT(TAIL_CANARY(allocList[pos]) == TAIL_CANARY_PATTERN);
        pos++;
        if (allocList[pos] == NULL) {
            break;
        }
    } while (pos < (sizeof(allocList) / sizeof(BlockLink_t*)));
}

void OPTIMIZE_FAST vPortUpdateFreeBlockList(void) {
    BlockLink_t *start = &xStart;
    do {
        HEAD_CANARY(start) = HEAD_CANARY_PATTERN;
        start = start->pxNextFreeBlock;
    } while(start->pxNextFreeBlock != NULL);
}

void OPTIMIZE_FAST vPortAddToList(uint32_t pointer) {
    uint16_t pos = 0;
    BlockLink_t *temp = (BlockLink_t*)pointer;
    HEAD_CANARY(temp) = HEAD_CANARY_PATTERN;
    TAIL_CANARY(temp) = TAIL_CANARY_PATTERN;
    do {
        if (allocList[pos] == NULL) {
            allocList[pos] = (BlockLink_t*)pointer;
            break;
        }
    } while (allocList[pos++] != NULL && pos < (sizeof(allocList) / sizeof(uint32_t)));
    vPortUpdateFreeBlockList();
}

uint8_t OPTIMIZE_FAST vPortRmFromList(uint32_t pointer) {
    uint16_t pos;
    for (pos = 0; pos < (sizeof(allocList) / sizeof(uint32_t)); pos++) {
        if (allocList[pos] == (BlockLink_t*)pointer) {
            allocList[pos] = NULL;
            break;
        }
    }
    if (pos == (sizeof(allocList) / sizeof(uint32_t))) {
        return 0xFF;
    }
    for (; pos < (sizeof(allocList) / sizeof(uint32_t) - 1); pos++) {
        allocList[pos] = allocList[pos + 1];
    }
    return 0;
}


extern int __start_data_RAM2;
extern int __end_data_RAM2;

extern int __start_data_RAM3;
extern int __end_data_RAM3;

typedef struct {
    uint32_t *startAddress;
    uint32_t *endAddress;
} tMemoryRegion;

tMemoryRegion ram_regions[2] = {
		{ .startAddress = (uint32_t *) (&__start_data_RAM3), .endAddress = (uint32_t *) (&__end_data_RAM3)},
		{ .startAddress = (uint32_t *) (&__start_data_RAM3), .endAddress = (uint32_t *) (&__end_data_RAM3)}};


static inline uint8_t isCurrentStackAddress(TaskHandle_t task, uint32_t *x) {
    TaskStatus_t status;
    vTaskGetInfo(task, &status, 0, 0);
    uint32_t *startHeap = status.pxStackBase;
    uint32_t *endHeap = task;
    if (x > startHeap && x < endHeap) {
        return 1;
    }
    return 0;
}

uint32_t OPTIMIZE_FAST vPortMemoryScan(void) {
    uint16_t pos = 0;
    uint32_t orphaned_buffers = 0;
    // Cycle through pointer array until null pointer is found
    while (allocList[pos] != NULL) {
        // Get address of allocated buffer that will be referenced in the memory
        uint32_t allocatedAddress = xHeapStructSize + (uint32_t)allocList[pos];
        // Get buffer owner
        TaskHandle_t xTask = (TaskHandle_t)allocList[pos]->xOwner;
        uint8_t found = 0;
        // Only check if there is buffer owner
        if (xTask != NULL) {
            // Get task info
            TaskStatus_t status;
            vTaskGetInfo(xTask, &status, 0, 0);
            // Calculate start address
            uint32_t *startAddress = status.pxStackBase;
            // Calculate end address
            uint32_t *endAddress = (uint32_t*)((uint32_t)xTask - xHeapStructSize);
            // Scan memory
            while (startAddress <= endAddress) {
                // Check if we have reference pointers
                if (*startAddress == allocatedAddress) {
                    // Break search
                    found = 1;
                    break;
                }
                // Increase address by 4
                startAddress += 4;
            }
            // Scan defined memory regions if we still don't have reference
            if (!found) {
                // Scan other mem region
                for (int i = 0; i < (sizeof ram_regions / sizeof ram_regions[0]); i++) {
                    // Calculate start address
                    startAddress = ram_regions[i].startAddress;
                    // Calculate end address
                    endAddress = ram_regions[i].endAddress;;
                    // Scan memory
                    while (startAddress < endAddress) {
                        // Check if we have reference pointers
                        if ((*startAddress == allocatedAddress) && (isCurrentStackAddress(xTaskGetCurrentTaskHandle(), startAddress) == 0)) {
                            found = 1;
                            break;
                        }
                        startAddress++;
                    }
                    if (found) break;
                }
                if (!found) {
                    // Didn't found any references to a pointer
                    orphaned_buffers++;
                    printf("Didn't find reference to buffer %lx (size %d) in task %s\n",
                            allocatedAddress, allocList[pos]->xBlockSize - xBlockAllocatedBit, status.pcTaskName);
                }
            }
        }
        pos++;
    }
    return orphaned_buffers;
}
#endif

/*************************
 *
 */


/*-----------------------------------------------------------*/

void *pvPortMalloc( size_t xWantedSize )
{
BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
void *pvReturn = NULL;

	vTaskSuspendAll();
	{
		/* If this is the first call to malloc then the heap will require
		initialisation to setup the list of free blocks. */
		if( pxEnd == NULL )
		{
			prvHeapInit();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if( xWantedSize > 0 )
			{
				xWantedSize += xHeapStructSize;
#if( configUSE_MALLOC_DEBUG == 1 )
                xWantedSize += sizeof(uint32_t); // add size of tail_canary
#endif

				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
				{
					/* Byte alignment required. */
					xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
					configASSERT( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) == 0 );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;
				while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if( pxBlock != pxEnd )
				{
					/* Return the memory space pointed to - jumping over the
					BlockLink_t structure at its start. */
					pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

#if( configUSE_MALLOC_DEBUG == 1 )
                    /* Current task owener */
                    if (xTaskGetCurrentTaskHandle() && xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
                        pxPreviousBlock->pxNextFreeBlock->xOwner = xTaskGetCurrentTaskHandle();
                    }
#endif

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

					/* If the block is larger than required it can be split into
					two. */
					if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );
						configASSERT( ( ( ( size_t ) pxNewBlockLink ) & portBYTE_ALIGNMENT_MASK ) == 0 );

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList( pxNewBlockLink );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					xFreeBytesRemaining -= pxBlock->xBlockSize;

					if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		traceMALLOC( pvReturn, xWantedSize );
	}
#if( configUSE_MALLOC_DEBUG == 1 )
    if (pvReturn != NULL) {
        vPortAddToList((uint32_t)pvReturn - xHeapStructSize);
    }
#endif
	( void ) xTaskResumeAll();

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	#endif

	configASSERT( ( ( ( size_t ) pvReturn ) & ( size_t ) portBYTE_ALIGNMENT_MASK ) == 0 );
	return pvReturn;
}

/*-----------------------------------------------------------*/

void vPortFree( void *pv )
{
uint8_t *puc = ( uint8_t * ) pv;
BlockLink_t *pxLink;

	if( pv != NULL )
	{
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= xHeapStructSize;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = ( void * ) puc;

		/* Check the block is actually allocated. */
		configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
		configASSERT( pxLink->pxNextFreeBlock == NULL );

		if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
		{
			if( pxLink->pxNextFreeBlock == NULL )
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				pxLink->xBlockSize &= ~xBlockAllocatedBit;

				vTaskSuspendAll();
				{
					/* Add this block to the list of free blocks. */
					xFreeBytesRemaining += pxLink->xBlockSize;
					traceFREE( pv, pxLink->xBlockSize );
#if( configUSE_MALLOC_DEBUG == 1 )
                    pxLink->xOwner = NULL;
                    vPortRmFromList((uint32_t)pxLink);
#endif
					prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
				}
#if( configUSE_MALLOC_DEBUG == 1 )
                vPortUpdateFreeBlockList();
#endif
				( void ) xTaskResumeAll();
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
	return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize( void )
{
	return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
	/* This just exists to keep the linker quiet. */
}
/*-----------------------------------------------------------*/

static void prvHeapInit( void )
{
BlockLink_t *pxFirstFreeBlock;
uint8_t *pucAlignedHeap;
size_t uxAddress;
size_t xTotalHeapSize = configTOTAL_HEAP_SIZE;

	/* Ensure the heap starts on a correctly aligned boundary. */
	uxAddress = ( size_t ) ucHeap;

	if( ( uxAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
	{
		uxAddress += ( portBYTE_ALIGNMENT - 1 );
		uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
		xTotalHeapSize -= uxAddress - ( size_t ) ucHeap;
	}

	pucAlignedHeap = ( uint8_t * ) uxAddress;

	/* xStart is used to hold a pointer to the first item in the list of free
	blocks.  The void cast is used to prevent compiler warnings. */
	xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
	xStart.xBlockSize = ( size_t ) 0;

	/* pxEnd is used to mark the end of the list of free blocks and is inserted
	at the end of the heap space. */
	uxAddress = ( ( size_t ) pucAlignedHeap ) + xTotalHeapSize;
	uxAddress -= xHeapStructSize;
	uxAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
	pxEnd = ( void * ) uxAddress;
	pxEnd->xBlockSize = 0;
	pxEnd->pxNextFreeBlock = NULL;

	/* To start with there is a single free block that is sized to take up the
	entire heap space, minus the space taken by pxEnd. */
	pxFirstFreeBlock = ( void * ) pucAlignedHeap;
	pxFirstFreeBlock->xBlockSize = uxAddress - ( size_t ) pxFirstFreeBlock;
	pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

	/* Only one block exists - and it covers the entire usable heap space. */
	xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
	xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;

	/* Work out the position of the top bit in a size_t variable. */
	xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
{
BlockLink_t *pxIterator;
uint8_t *puc;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxIterator;
	if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
	{
		if( pxIterator->pxNextFreeBlock != pxEnd )
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
		}
		else
		{
			pxBlockToInsert->pxNextFreeBlock = pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextFreeBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if( pxIterator != pxBlockToInsert )
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}
}


/*
 * GENERATED FILE - DO NOT EDIT
 * Copyright (c) 2008-2013 Code Red Technologies Ltd,
 * Copyright 2015, 2018-2019 NXP
 * (c) NXP Semiconductors 2013-2021
 * Generated linker script file for LPC4337
 * Created from memory.ldt by FMCreateLinkMemory
 * Using Freemarker v2.3.23
 * MCUXpresso IDE v11.2.0 [Build 4120] [2020-07-09] on 18/06/2021 22:06:26
 */

MEMORY
{
  /* Define each memory region */
  MFlashA512 (rx) : ORIGIN = 0x1a000000, LENGTH = 0x80000 /* 512K bytes (alias Flash) */  
  MFlashB512 (rx) : ORIGIN = 0x1b000000, LENGTH = 0x80000 /* 512K bytes (alias Flash2) */  
  RamLoc32 (rwx) : ORIGIN = 0x10000000, LENGTH = 0x8000 /* 32K bytes (alias RAM) */  
  RamLoc40 (rwx) : ORIGIN = 0x10080000, LENGTH = 0xa000 /* 40K bytes (alias RAM2) */  
  RamAHB32_48_64 (rwx) : ORIGIN = 0x20000000, LENGTH = 0x10000 /* 64K bytes (alias RAM3) */  
}

  /* Define a symbol for the top of each memory region */
  __base_MFlashA512 = 0x1a000000  ; /* MFlashA512 */  
  __base_Flash = 0x1a000000 ; /* Flash */  
  __top_MFlashA512 = 0x1a000000 + 0x80000 ; /* 512K bytes */  
  __top_Flash = 0x1a000000 + 0x80000 ; /* 512K bytes */  
  __base_MFlashB512 = 0x1b000000  ; /* MFlashB512 */  
  __base_Flash2 = 0x1b000000 ; /* Flash2 */  
  __top_MFlashB512 = 0x1b000000 + 0x80000 ; /* 512K bytes */  
  __top_Flash2 = 0x1b000000 + 0x80000 ; /* 512K bytes */  
  __base_RamLoc32 = 0x10000000  ; /* RamLoc32 */  
  __base_RAM = 0x10000000 ; /* RAM */  
  __top_RamLoc32 = 0x10000000 + 0x8000 ; /* 32K bytes */  
  __top_RAM = 0x10000000 + 0x8000 ; /* 32K bytes */  
  __base_RamLoc40 = 0x10080000  ; /* RamLoc40 */  
  __base_RAM2 = 0x10080000 ; /* RAM2 */  
  __top_RamLoc40 = 0x10080000 + 0xa000 ; /* 40K bytes */  
  __top_RAM2 = 0x10080000 + 0xa000 ; /* 40K bytes */  
  __base_RamAHB32_48_64 = 0x20000000  ; /* RamAHB32_48_64 */  
  __base_RAM3 = 0x20000000 ; /* RAM3 */  
  __top_RamAHB32_48_64 = 0x20000000 + 0x10000 ; /* 64K bytes */  
  __top_RAM3 = 0x20000000 + 0x10000 ; /* 64K bytes */  

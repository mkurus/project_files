/*
 * GENERATED FILE - DO NOT EDIT
 * (c) Code Red Technologies Ltd, 2008-2013
 * (c) NXP Semiconductors 2013-2017
 * Generated linker script file for LPC1768
 * Created from memory.ldt by FMCreateLinkMemory
 * Using Freemarker v2.3.23
 * LPCXpresso v8.2.2 [Build 650] [2016-09-09]  on 27.Oca.2017 10:06:32
 */

MEMORY
{
  /* Define each memory region */
  MFlash512 (rx) : ORIGIN = 0x10000, LENGTH = 0x30000 /* 192K bytes (alias Flash) */  
  RamLoc32 (rwx) : ORIGIN = 0x10000000, LENGTH = 0x7fc0 /* 32704 bytes (alias RAM) */  
  RamAHB32 (rwx) : ORIGIN = 0x2007c000, LENGTH = 0x8000 /* 32K bytes (alias RAM2) */  
}

  /* Define a symbol for the top of each memory region */
  __base_MFlash512 = 0x10000  ; /* MFlash512 */  
  __base_Flash = 0x10000 ; /* Flash */  
  __top_MFlash512 = 0x10000 + 0x30000 ; /* 192K bytes */  
  __top_Flash = 0x10000 + 0x30000 ; /* 192K bytes */  
  __base_RamLoc32 = 0x10000000  ; /* RamLoc32 */  
  __base_RAM = 0x10000000 ; /* RAM */  
  __top_RamLoc32 = 0x10000000 + 0x7fc0 ; /* 32704 bytes */  
  __top_RAM = 0x10000000 + 0x7fc0 ; /* 32704 bytes */  
  __base_RamAHB32 = 0x2007c000  ; /* RamAHB32 */  
  __base_RAM2 = 0x2007c000 ; /* RAM2 */  
  __top_RamAHB32 = 0x2007c000 + 0x8000 ; /* 32K bytes */  
  __top_RAM2 = 0x2007c000 + 0x8000 ; /* 32K bytes */  

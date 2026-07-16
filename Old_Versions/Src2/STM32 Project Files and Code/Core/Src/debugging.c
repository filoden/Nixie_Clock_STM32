/*
 * debugging.c
 *
 *  Created on: Jan 9, 2026
 *      Author: Apath
 */
#ifndef DEBUGGIN
#define DEBUGGIN
#include "usart.h"
#include <stdio.h> // used for DEBUG only
#include <string.h> // used for DEBUG UART handling
#include "stm32g0xx_hal.h"
#include <string.h>
#include "app_fatfs.h"

int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, 100);
  return len;
}
extern UART_HandleTypeDef huart2;

void uart_print_u32(uint32_t x)
{
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "%lu", (unsigned long)x);
    if (n > 0) {
        HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)n, 100);
    }
}

void printstr(char str[]){
    HAL_UART_Transmit(&huart2, (uint8_t*)str, (uint16_t)strlen(str), 100);
}




void debugnote(char str[]){
	char buff[256];
	FATFS fs;
	FIL fil;
	strcat(buff, str);
	f_mount(&fs, "0:", 1);
	f_open(&fil, "0:/debug.txt", FA_OPEN_APPEND | FA_WRITE);
	f_puts(buff, &fil);
	f_close(&fil);
}

void debugclr(char str[]){
	FATFS fs;
	FIL fil;
	f_mount(&fs, "0:", 1);
	f_open(&fil, "0:/debug.txt", FA_OPEN_ALWAYS | FA_WRITE);
	f_truncate(&fil);
	f_close(&fil);
}
#endif

// FatFs error helpers for STM32 (uses printstr())
// Include ff.h (or fatfs.h depending on your setup) for FRESULT / FR_*.

#include "ff.h"   // or "fatfs.h" in some CubeMX setups

static void print_fr_common(FRESULT fr)
{
    switch (fr)
    {
    case FR_OK:                 printstr("FR_OK: succeeded"); break;
    case FR_DISK_ERR:           printstr("FR_DISK_ERR: low-level disk I/O error (SPI/SD comms, power, wiring, CRC/timeouts)"); break;
    case FR_INT_ERR:            printstr("FR_INT_ERR: internal FatFs error"); break;
    case FR_NOT_READY:          printstr("FR_NOT_READY: drive not ready (not initialized / no card / power / CS)"); break;
    case FR_NO_FILE:            printstr("FR_NO_FILE: file not found"); break;
    case FR_NO_PATH:            printstr("FR_NO_PATH: path not found"); break;
    case FR_INVALID_NAME:       printstr("FR_INVALID_NAME: invalid path/name"); break;
    case FR_DENIED:             printstr("FR_DENIED: access denied (permission, read-only, directory full, etc.)"); break;
    case FR_EXIST:              printstr("FR_EXIST: file already exists"); break;
    case FR_INVALID_OBJECT:     printstr("FR_INVALID_OBJECT: invalid file/dir object (bad FIL pointer, already closed, not opened)"); break;
    case FR_WRITE_PROTECTED:    printstr("FR_WRITE_PROTECTED: media is write-protected"); break;
    case FR_INVALID_DRIVE:      printstr("FR_INVALID_DRIVE: invalid logical drive number"); break;
    case FR_NOT_ENABLED:        printstr("FR_NOT_ENABLED: volume has no work area / not enabled"); break;
    case FR_NO_FILESYSTEM:      printstr("FR_NO_FILESYSTEM: no valid FAT volume found (not formatted / wrong FS)"); break;
    case FR_MKFS_ABORTED:       printstr("FR_MKFS_ABORTED: mkfs aborted"); break;
    case FR_TIMEOUT:            printstr("FR_TIMEOUT: timeout waiting for the drive"); break;
    case FR_LOCKED:             printstr("FR_LOCKED: file locked (re-entrancy / sharing violation)"); break;
    case FR_NOT_ENOUGH_CORE:    printstr("FR_NOT_ENOUGH_CORE: out of memory (FatFs work area)"); break;
    case FR_TOO_MANY_OPEN_FILES:printstr("FR_TOO_MANY_OPEN_FILES: too many open files"); break;
    case FR_INVALID_PARAMETER:  printstr("FR_INVALID_PARAMETER: bad parameter"); break;
    default:                    printstr("Unknown FRESULT code"); break;
    }
}

// --- Requested per-API wrappers ---
// Note: If your code currently stores the return in uint8_t (like `uint8_t fr = f_open(...)`),
// this still works because we cast to FRESULT.

void print_fopen_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_open] ");
    print_fr_common(fr);

    // Small, open-specific nudges
    if (fr == FR_INVALID_NAME)  printstr("\n\rHint: check \"0:/path/file.ext\" formatting and drive string.");
    if (fr == FR_NO_PATH)       printstr("\n\rHint: a folder in the path doesn't exist.");
    if (fr == FR_DENIED)        printstr("\n\rHint: opening with write flags on a read-only file/dir can cause this.");
    if (fr == FR_INVALID_DRIVE) printstr("\n\rHint: make sure the drive number matches your diskio driver (often 0:).");
}

void print_fread_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_read] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_OBJECT) printstr("\n\rHint: FIL is not opened or got corrupted (stack lifetime, double-close, etc.).");
    if (fr == FR_DISK_ERR)       printstr("\n\rHint: check SPI clock rate, pullups, wiring, and card power integrity.");
    if (fr == FR_TIMEOUT)        printstr("\n\rHint: card may be busy or SPI is stalling; try lower SPI baud / check CS timing.");
}

void print_fwrite_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_write] ");
    print_fr_common(fr);

    if (fr == FR_DENIED)          printstr("\n\rHint: file not opened with write access, or filesystem policy denies the write.");
    if (fr == FR_WRITE_PROTECTED) printstr("\n\rHint: card is write-protected or mounted read-only.");
    if (fr == FR_DISK_ERR)        printstr("\n\rHint: common causes are SD comms issues or insufficient power during writes.");
    if (fr == FR_TIMEOUT)         printstr("\n\rHint: card busy too long; try lower SPI baud or ensure proper polling/busy-waits.");
}

void print_fmount_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_mount] ");
    print_fr_common(fr);

    if (fr == FR_NO_FILESYSTEM)  printstr("\n\rHint: card not formatted FAT/FAT32 (or needs reformat).");
    if (fr == FR_INVALID_DRIVE)  printstr("\n\rHint: wrong drive string; usually \"0:\" for the first SD diskio.");
    if (fr == FR_NOT_READY)      printstr("\n\rHint: disk_initialize likely failing (card detect, power, CS, SPI).");
}

void print_fclose_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_close] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_OBJECT) printstr("\n\rHint: closing a file that was never opened or already closed.");
    if (fr == FR_DISK_ERR)       printstr("\n\rHint: close flushes pending data; write-side SD issues can surface here.");
}

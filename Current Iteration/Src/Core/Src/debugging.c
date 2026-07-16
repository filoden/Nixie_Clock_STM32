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
#include "UserInterface.h"

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
	HAL_StatusTypeDef err = HAL_UART_Transmit(&huart2, (uint8_t*)str, (uint16_t)strlen(str), 100);

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
void print_fopen_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_open] ");
    print_fr_common(fr);

    if (fr == FR_NO_FILE)          printstr("\n\rHint: file does not exist; check spelling, extension, and current directory.");
    if (fr == FR_NO_PATH)          printstr("\n\rHint: directory path does not exist.");
    if (fr == FR_INVALID_NAME)     printstr("\n\rHint: invalid filename/path format.");
    if (fr == FR_DENIED)           printstr("\n\rHint: access denied; file may already exist, directory/full, or mode flags conflict.");
    if (fr == FR_EXIST)            printstr("\n\rHint: file already exists and create-new mode was requested.");
    if (fr == FR_INVALID_OBJECT)   printstr("\n\rHint: FIL object may be corrupt or reused incorrectly.");
    if (fr == FR_NOT_ENABLED)      printstr("\n\rHint: volume not mounted; call f_mount first.");
    if (fr == FR_NO_FILESYSTEM)    printstr("\n\rHint: volume is not FAT/FAT32/exFAT or mount failed.");
    if (fr == FR_DISK_ERR)         printstr("\n\rHint: low-level SD/SPI read problem while opening file.");
    if (fr == FR_TIMEOUT)          printstr("\n\rHint: filesystem lock timed out.");
    if (fr == FR_TOO_MANY_OPEN_FILES) printstr("\n\rHint: too many files open; increase _FS_LOCK or close files.");
}

void print_fread_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_read] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_OBJECT) printstr("\n\rHint: file object invalid; f_open may have failed or FIL object is corrupt.");
    if (fr == FR_DENIED)         printstr("\n\rHint: file not opened with read access.");
    if (fr == FR_DISK_ERR)       printstr("\n\rHint: SD/SPI read failed, timeout, bad token, or power/signal issue.");
    if (fr == FR_TIMEOUT)        printstr("\n\rHint: filesystem lock timed out during read.");
}

void print_flseek_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_lseek] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_OBJECT)    printstr("\n\rHint: file object invalid; f_open may have failed.");
    if (fr == FR_INVALID_PARAMETER) printstr("\n\rHint: seek offset invalid or outside supported range.");
    if (fr == FR_DENIED)            printstr("\n\rHint: seek may require file expansion but write access is denied.");
    if (fr == FR_DISK_ERR)          printstr("\n\rHint: SD read error while following cluster chain.");
    if (fr == FR_TIMEOUT)           printstr("\n\rHint: filesystem lock timed out during seek.");
}

void print_ftruncate_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_truncate] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_OBJECT) printstr("\n\rHint: file object invalid.");
    if (fr == FR_DENIED)         printstr("\n\rHint: file not opened with write access.");
    if (fr == FR_WRITE_PROTECTED) printstr("\n\rHint: card or filesystem is write-protected.");
    if (fr == FR_DISK_ERR)       printstr("\n\rHint: SD write/update error while truncating file.");
    if (fr == FR_TIMEOUT)        printstr("\n\rHint: filesystem lock timed out.");
}

void print_fsync_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_sync] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_OBJECT)  printstr("\n\rHint: file object invalid.");
    if (fr == FR_WRITE_PROTECTED) printstr("\n\rHint: card or filesystem is write-protected.");
    if (fr == FR_DISK_ERR)        printstr("\n\rHint: pending write/metadata flush failed.");
    if (fr == FR_TIMEOUT)         printstr("\n\rHint: card or filesystem lock busy too long.");
}

void print_fopendir_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_opendir] ");
    print_fr_common(fr);

    if (fr == FR_NO_PATH)       printstr("\n\rHint: directory path does not exist.");
    if (fr == FR_INVALID_NAME)  printstr("\n\rHint: invalid directory path string.");
    if (fr == FR_NOT_ENABLED)   printstr("\n\rHint: volume not mounted.");
    if (fr == FR_NO_FILESYSTEM) printstr("\n\rHint: volume does not contain a valid FAT filesystem.");
    if (fr == FR_DISK_ERR)      printstr("\n\rHint: SD read error while opening directory.");
    if (fr == FR_TIMEOUT)       printstr("\n\rHint: filesystem lock timed out.");
}

void print_fclosedir_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_closedir] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_OBJECT) printstr("\n\rHint: directory object invalid or already closed.");
}

void print_freaddir_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_readdir] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_OBJECT) printstr("\n\rHint: directory object invalid; f_opendir may have failed.");
    if (fr == FR_DISK_ERR)       printstr("\n\rHint: SD read error while reading directory entry.");
    if (fr == FR_TIMEOUT)        printstr("\n\rHint: filesystem lock timed out.");
}

void print_ffindfirst_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_findfirst] ");
    print_fr_common(fr);

    if (fr == FR_NO_PATH)       printstr("\n\rHint: search directory path does not exist.");
    if (fr == FR_INVALID_NAME)  printstr("\n\rHint: invalid path or search pattern.");
    if (fr == FR_NOT_ENABLED)   printstr("\n\rHint: volume not mounted.");
    if (fr == FR_DISK_ERR)      printstr("\n\rHint: SD read error while starting search.");
    if (fr == FR_TIMEOUT)       printstr("\n\rHint: filesystem lock timed out.");
}

void print_ffindnext_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_findnext] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_OBJECT) printstr("\n\rHint: search/directory object invalid.");
    if (fr == FR_DISK_ERR)       printstr("\n\rHint: SD read error while continuing search.");
    if (fr == FR_TIMEOUT)        printstr("\n\rHint: filesystem lock timed out.");
}

void print_fmkdir_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_mkdir] ");
    print_fr_common(fr);

    if (fr == FR_EXIST)           printstr("\n\rHint: directory or file already exists.");
    if (fr == FR_NO_PATH)         printstr("\n\rHint: parent directory does not exist.");
    if (fr == FR_INVALID_NAME)    printstr("\n\rHint: invalid directory name.");
    if (fr == FR_DENIED)          printstr("\n\rHint: directory cannot be created; root/full/permission issue.");
    if (fr == FR_WRITE_PROTECTED) printstr("\n\rHint: card or filesystem is write-protected.");
    if (fr == FR_DISK_ERR)        printstr("\n\rHint: SD write error while creating directory.");
}

void print_funlink_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_unlink] ");
    print_fr_common(fr);

    if (fr == FR_NO_FILE)         printstr("\n\rHint: file or directory does not exist.");
    if (fr == FR_NO_PATH)         printstr("\n\rHint: parent path does not exist.");
    if (fr == FR_INVALID_NAME)    printstr("\n\rHint: invalid filename/path.");
    if (fr == FR_DENIED)          printstr("\n\rHint: object may be read-only, non-empty directory, or protected.");
    if (fr == FR_WRITE_PROTECTED) printstr("\n\rHint: card or filesystem is write-protected.");
    if (fr == FR_DISK_ERR)        printstr("\n\rHint: SD write/update error while deleting object.");
}

void print_frename_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_rename] ");
    print_fr_common(fr);

    if (fr == FR_NO_FILE)         printstr("\n\rHint: source file/directory does not exist.");
    if (fr == FR_NO_PATH)         printstr("\n\rHint: source or destination path does not exist.");
    if (fr == FR_INVALID_NAME)    printstr("\n\rHint: invalid source or destination name.");
    if (fr == FR_EXIST)           printstr("\n\rHint: destination already exists.");
    if (fr == FR_DENIED)          printstr("\n\rHint: source/destination denied, open, read-only, or invalid move.");
    if (fr == FR_WRITE_PROTECTED) printstr("\n\rHint: card or filesystem is write-protected.");
    if (fr == FR_DISK_ERR)        printstr("\n\rHint: SD write/update error while renaming.");
}

void print_fstat_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_stat] ");
    print_fr_common(fr);

    if (fr == FR_NO_FILE)       printstr("\n\rHint: file or directory does not exist.");
    if (fr == FR_NO_PATH)       printstr("\n\rHint: path does not exist.");
    if (fr == FR_INVALID_NAME)  printstr("\n\rHint: invalid path/name.");
    if (fr == FR_NOT_ENABLED)   printstr("\n\rHint: volume not mounted.");
    if (fr == FR_NO_FILESYSTEM) printstr("\n\rHint: volume has no valid filesystem.");
    if (fr == FR_DISK_ERR)      printstr("\n\rHint: SD read error while reading file info.");
}

void print_fchmod_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_chmod] ");
    print_fr_common(fr);

    if (fr == FR_NO_FILE)         printstr("\n\rHint: target file/directory does not exist.");
    if (fr == FR_NO_PATH)         printstr("\n\rHint: path does not exist.");
    if (fr == FR_INVALID_NAME)    printstr("\n\rHint: invalid path/name.");
    if (fr == FR_WRITE_PROTECTED) printstr("\n\rHint: card or filesystem is write-protected.");
    if (fr == FR_DISK_ERR)        printstr("\n\rHint: SD write/update error while changing attributes.");
}

void print_futime_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_utime] ");
    print_fr_common(fr);

    if (fr == FR_NO_FILE)         printstr("\n\rHint: target file/directory does not exist.");
    if (fr == FR_NO_PATH)         printstr("\n\rHint: path does not exist.");
    if (fr == FR_INVALID_NAME)    printstr("\n\rHint: invalid path/name.");
    if (fr == FR_WRITE_PROTECTED) printstr("\n\rHint: card or filesystem is write-protected.");
    if (fr == FR_DISK_ERR)        printstr("\n\rHint: SD write/update error while changing timestamp.");
}

void print_fchdir_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_chdir] ");
    print_fr_common(fr);

    if (fr == FR_NO_PATH)       printstr("\n\rHint: directory path does not exist.");
    if (fr == FR_INVALID_NAME)  printstr("\n\rHint: invalid directory path.");
    if (fr == FR_NOT_ENABLED)   printstr("\n\rHint: volume not mounted.");
    if (fr == FR_NO_FILESYSTEM) printstr("\n\rHint: volume has no valid filesystem.");
    if (fr == FR_DISK_ERR)      printstr("\n\rHint: SD read error while changing directory.");
}

void print_fchdrive_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_chdrive] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_DRIVE) printstr("\n\rHint: invalid drive number/string; usually use \"0:\" for first SD volume.");
    if (fr == FR_NOT_ENABLED)   printstr("\n\rHint: target volume not mounted/enabled.");
}

void print_fgetcwd_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_getcwd] ");
    print_fr_common(fr);

    if (fr == FR_NOT_ENABLED)       printstr("\n\rHint: volume not mounted.");
    if (fr == FR_NO_FILESYSTEM)     printstr("\n\rHint: volume has no valid filesystem.");
    if (fr == FR_INVALID_DRIVE)     printstr("\n\rHint: invalid current drive.");
    if (fr == FR_NOT_ENOUGH_CORE)   printstr("\n\rHint: provided string buffer may be too small or LFN working buffer issue.");
    if (fr == FR_DISK_ERR)          printstr("\n\rHint: SD read error while building current path.");
}

void print_fgetfree_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_getfree] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_DRIVE) printstr("\n\rHint: wrong drive string; usually \"0:\".");
    if (fr == FR_NOT_ENABLED)   printstr("\n\rHint: volume not mounted.");
    if (fr == FR_NO_FILESYSTEM) printstr("\n\rHint: volume has no valid FAT filesystem.");
    if (fr == FR_DISK_ERR)      printstr("\n\rHint: SD read error while scanning FAT/free clusters.");
}

void print_fgetlabel_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_getlabel] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_DRIVE) printstr("\n\rHint: wrong drive string; usually \"0:\".");
    if (fr == FR_NOT_ENABLED)   printstr("\n\rHint: volume not mounted.");
    if (fr == FR_NO_FILESYSTEM) printstr("\n\rHint: volume has no valid FAT filesystem.");
    if (fr == FR_DISK_ERR)      printstr("\n\rHint: SD read error while reading volume label.");
}

void print_fsetlabel_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_setlabel] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_NAME)    printstr("\n\rHint: invalid volume label string.");
    if (fr == FR_WRITE_PROTECTED) printstr("\n\rHint: card or filesystem is write-protected.");
    if (fr == FR_NOT_ENABLED)     printstr("\n\rHint: volume not mounted.");
    if (fr == FR_NO_FILESYSTEM)   printstr("\n\rHint: volume has no valid FAT filesystem.");
    if (fr == FR_DISK_ERR)        printstr("\n\rHint: SD write/update error while setting volume label.");
}

void print_fforward_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_forward] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_OBJECT) printstr("\n\rHint: file object invalid.");
    if (fr == FR_DENIED)         printstr("\n\rHint: file not opened for reading.");
    if (fr == FR_DISK_ERR)       printstr("\n\rHint: SD read error during forward callback streaming.");
    if (fr == FR_TIMEOUT)        printstr("\n\rHint: filesystem lock timed out.");
}

void print_fexpand_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_expand] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_OBJECT) printstr("\n\rHint: file object invalid.");
    if (fr == FR_DENIED)         printstr("\n\rHint: file not opened for write, not empty, or not enough contiguous space.");
    if (fr == FR_DISK_ERR)       printstr("\n\rHint: SD/FAT update error while allocating file space.");
    if (fr == FR_TIMEOUT)        printstr("\n\rHint: filesystem lock timed out.");
}

void print_fmkfs_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_mkfs] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_DRIVE)     printstr("\n\rHint: wrong drive string; usually \"0:\".");
    if (fr == FR_NOT_READY)         printstr("\n\rHint: disk_initialize failed; check card power, CS, SPI, detect.");
    if (fr == FR_WRITE_PROTECTED)   printstr("\n\rHint: card is write-protected.");
    if (fr == FR_MKFS_ABORTED)      printstr("\n\rHint: mkfs aborted; invalid parameters or unsuitable volume size.");
    if (fr == FR_NOT_ENOUGH_CORE)   printstr("\n\rHint: work buffer too small.");
    if (fr == FR_INVALID_PARAMETER) printstr("\n\rHint: invalid format option, allocation unit, or work buffer size.");
    if (fr == FR_DISK_ERR)          printstr("\n\rHint: SD read/write error while formatting.");
}

void print_ffdisk_error(uint8_t err)
{
    FRESULT fr = (FRESULT)err;
    printstr("\n\r[f_fdisk] ");
    print_fr_common(fr);

    if (fr == FR_INVALID_DRIVE)     printstr("\n\rHint: invalid physical drive number.");
    if (fr == FR_NOT_READY)         printstr("\n\rHint: disk_initialize failed.");
    if (fr == FR_WRITE_PROTECTED)   printstr("\n\rHint: card is write-protected.");
    if (fr == FR_INVALID_PARAMETER) printstr("\n\rHint: invalid partition table parameters or work buffer.");
    if (fr == FR_DISK_ERR)          printstr("\n\rHint: SD write error while writing partition table.");
}

void Interrupt_Debug(){
	uint8_t interruptHist[100] = {0};
	uint8_t intvar = 0;
	int i = 0;
	while (1){
		//HAL_Delay(100);
		if(USER_INT_REG0 != 0){
			intvar = USER_INT_REG0;
			USER_INT_REG0 = 0;
			}
		else if (USER_INT_REG1 != 0){
			intvar = USER_INT_REG1;
			USER_INT_REG1 = 0;
			}
		else if (USER_INT_REG2 != 0){
			intvar = USER_INT_REG2;
			USER_INT_REG2 = 0;
			}
		else if (USER_INT_REG3 != 0){
			intvar = USER_INT_REG3;
			USER_INT_REG3 = 0;
			}
		else if (USER_INT_REG4 != 0){
			intvar = USER_INT_REG4;
			USER_INT_REG4 = 0;
			}
		else if (USER_INT_REG5 != 0){
			intvar = USER_INT_REG5;
			USER_INT_REG5 = 0;
			}
		else if (USER_INT_REG6 != 0){
			intvar = USER_INT_REG6;
			USER_INT_REG6 = 0;
			}
		else if (USER_INT_REG7 != 0){
			intvar = USER_INT_REG7;
			USER_INT_REG7 = 0;
			}
		if (intvar != 0){
			interruptHist[i] = intvar;
			switch (intvar){
				case (0b10000001):
					printstr("KNA L\n\r");
					break;
				case (0b10000010):
					printstr("KNA R\n\r");
					break;
				case (0b10000100):
					printstr("KNB L\n\r");
					break;
				case (0b10001000):
					printstr("KNB R\n\r");
					break;
				case (0b10010000):
					printstr("KNC L\n\r");
					break;
				case (0b10100000):
					printstr("KNC R\n\r");
					break;
				case (0b11000000):
					printstr("PB\n\r");
					break;
				default:
					printstr("Error\n\r");
			}
			intvar = 0;
			i++;
			i = i % 100;
		}
	}
}





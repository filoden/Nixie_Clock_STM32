/*
 * debugging.h
 *
 *  Created on: Jan 9, 2026
 *      Author: Apath
 */

#ifndef INC_DEBUGGING_H_
#define INC_DEBUGGING_H_
#include "ff.h"
int _write(int file, char *ptr, int len);
void uart_print_u32(uint32_t x);
void printstr(char str[]);
void debugnote(char str[]);



// FatFs error helpers for STM32 (uses printstr())
// Include ff.h (or fatfs.h depending on your setup) for FRESULT / FR_*.
void print_fopen_error(uint8_t err);
void print_fread_error(uint8_t err);
void print_fwrite_error(uint8_t err);
void print_fmount_error(uint8_t err);
void print_fclose_error(uint8_t err);
void print_flseek_error(uint8_t err);
void print_ftruncate_error(uint8_t err);
void print_fsync_error(uint8_t err);
void print_fopendir_error(uint8_t err);
void print_fclosedir_error(uint8_t err);
void print_freaddir_error(uint8_t err);
void print_ffindfirst_error(uint8_t err);
void print_ffindnext_error(uint8_t err);
void print_fmkdir_error(uint8_t err);
void print_funlink_error(uint8_t err);
void print_frename_error(uint8_t err);
void print_fstat_error(uint8_t err);
void print_fchmod_error(uint8_t err);
void print_futime_error(uint8_t err);
void print_fchdir_error(uint8_t err);
void print_fchdrive_error(uint8_t err);
void print_fgetcwd_error(uint8_t err);
void print_fgetfree_error(uint8_t err);
void print_fgetlabel_error(uint8_t err);
void print_fsetlabel_error(uint8_t err);
void print_fforward_error(uint8_t err);
void print_fexpand_error(uint8_t err);
void print_fmkfs_error(uint8_t err);
void print_ffdisk_error(uint8_t err);
void Interrupt_Debug();

#endif /* INC_DEBUGGING_H_ */

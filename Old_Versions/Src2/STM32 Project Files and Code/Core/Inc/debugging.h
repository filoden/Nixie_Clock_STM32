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

#endif /* INC_DEBUGGING_H_ */

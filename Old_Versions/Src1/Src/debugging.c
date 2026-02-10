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
void d1(){
	  const char msg[] = " -- Debug 1 -- !\r\n";
	  HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
}
void d2(){
	  const char msg[] = " -- Debug 2 -- !\r\n";
	  HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
}
void d3(){
	  const char msg[] = " -- Debug 3 -- !\r\n";
	  HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
}
void d4(){
	  const char msg[] = " -- Debug 4 -- !\r\n";
	  HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
}
void d5(){
	  const char msg[] = " -- Debug 5 -- !\r\n";
	  HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
}
void d6(){
	  const char msg[] = " -- Debug 5 -- !\r\n";
	  HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
}
void d7(){
	  const char msg[] = " -- Debug 5 -- !\r\n";
	  HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
}
void i1(){
	  const char msg[] = " -- interrupt 1 -- !\r\n";
	  HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
}
void i2(){
	  const char msg[] = " -- interrupt 2 -- !\r\n";
	  HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
}
void i3(){
	  const char msg[] = " -- interrupt 3 -- !\r\n";
	  HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
}
void i4(){
	  const char msg[] = " -- interrupt 4 -- !\r\n";
	  HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, 100);
}
#endif


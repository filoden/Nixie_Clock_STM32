/*
 * display.h
 *
 *  Created on: Feb 1, 2026
 *      Author: Apath
 */

#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_
#include <stdint.h>

void nixieDisp(uint16_t data);

void write_LE_N(uint8_t mode);

void write_CLK(uint8_t mode){;

void write_DIN(uint8_t mode);

void write_BL_N(uint8_t mode);


#endif /* INC_DISPLAY_H_ */

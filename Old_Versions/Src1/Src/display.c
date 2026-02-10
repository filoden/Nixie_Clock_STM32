/*
 * display.c
 *
 *  Created on: Feb 1, 2026
 *      Author: Apath
 */

/* modes:
 * 0 - phase in display
 * 1 - phase change display
 * 2 - blinking slow
 * 3 - blinking medium
 * 4 - blinking fast
 * 5 - cathode de-poison routine
 */
#include "gpio.h"

#define LE_PIN	GPIO_PIN_3 // PB3
#define LE_PORT GPIOB
#define CLK_PIN GPIO_PIN_1  // PA1
#define CLK_PORT GPIOA
#define DIN_PIN GPIO_PIN_15  // PA15
#define DIN_PORT GPIOA
#define BL_PIN GPIO_PIN_2 // PB2
#define BL_PORT GPIOB
#define CLK_PULSE 10

 // sends data to nixie for immediate display
 // 40 lines which correspond to each pin (0-9) on each tube (A-D)


void nixieDisp(uint16_t data, uint8_t blank){ // data contains the exact value to be displayed i.e. "1000" = "10:00" // 4LSB correspond to blanking each tube, i.e. 0b1111 = BLANK ALL, 0b1001 = BLANK tube A / D
// tube A - MSB (Big hour digit)
// tube B - Small hour digit
// tube C - Big minute digit
// tube D - Small minute digit
// each pin on the HV5622 corresponds to a pin on the nixie tube - mapped here: HVOUT1 = 0A, HVOUT2 = 1A, HVOUT11 = 0B, HVOUT12 = 1B, etc.
// Since this is a Serial to parallel data stream, each also corresponds to an offset in the data stream which is equal to (HVOUT# - 1)
	uint64_t ds;
	packageDs(data, &ds);
	write_LE(0);
	for (uint8_t i = 0; i<64; i++){
		if (i>23){
			write_DIN(ds >> i);
			write_CLK(1);
			HAL_Delay(CLK_PULSE);
			write_CLK(0);
		}
		else{
			write_DIN(0);
			write_CLK(1);
			HAL_Delay(CLK_PULSE);
			write_CLK(0);
		}
	}
}




void write_LE(uint8_t mode){
	switch (mode){
	case 0: //clear
		HAL_GPIO_WritePin(LE_PORT, LE_PIN, GPIO_PIN_RESET);
		break;
	case 1: //assert
		HAL_GPIO_WritePin(LE_PORT, LE_PIN, GPIO_PIN_SET);
		break;
	case 2: // toggle
		HAL_GPIO_TogglePin(LE_PORT, LE_PIN);
		break;
	}
}

void write_CLK(uint8_t mode){
	switch (mode){
	case 0: //clear
		HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);
		break;
	case 1: //assert
		HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);
		break;
	case 2: // toggle
		HAL_GPIO_TogglePin(CLK_PORT, CLK_PIN);
		break;
	}

}

void write_DIN(uint8_t mode){
	switch (mode){
	case 0: //clear
		HAL_GPIO_WritePin(DIN_PORT, DIN_PIN, GPIO_PIN_RESET);
		break;
	case 1: //assert
		HAL_GPIO_WritePin(DIN_PORT, DIN_PIN, GPIO_PIN_SET);
		break;
	case 2: // toggle
		HAL_GPIO_TogglePin(DIN_PORT, DIN_PIN);
		break;
	}

}

void write_BL(uint8_t mode){

	switch (mode){
	case 0: //clear
		HAL_GPIO_WritePin(BL_PORT, BL_PIN, GPIO_PIN_RESET);
		break;
	case 1: //assert
		HAL_GPIO_WritePin(BL_PORT, BL_PIN, GPIO_PIN_SET);
		break;
	case 2: // toggle
		HAL_GPIO_TogglePin(BL_PORT, BL_PIN);
		break;
	}

}

void packageDs(uint16_t data, uint64_t *ds)
{
    *ds = 0ULL;

    uint8_t digit1 =  data         % 10u;
    uint8_t digit2 = (data / 10u)  % 10u;
    uint8_t digit3 = (data / 100u) % 10u;
    uint8_t digit4 = (data / 1000u)% 10u;

    *ds |= (1ULL << digit1);
    *ds |= (1ULL << (digit2 + 10u));
    *ds |= (1ULL << (digit3 + 20u));
    *ds |= (1ULL << (digit4 + 30u));

    uint8_t found = 0;
    for (uint8_t i = 0; i < 40u; i++) {

        if ((i % 10u) == 0u) {
            found = 0;
        }

        uint8_t test = (uint8_t)((*ds >> i) & 1ULL);
        if (test) {
            found++;
            if (found > 1u) {
                printstr("Critical Failure: Multiple digits driven at once.");
            }
        }
    }
}

/*
 * cap1206.c
 *
 *  Created on: Jan 12, 2026
 *      Author: Apath
 */


// CAP1206 - 6 channel capacitive touch sensor - register association
#include <stdint.h>
#include <stdlib.h>
#include "debugging.h"
#include "i2c.h"
#include "cap1206.h"
// CAP1206 register addresses (see datasheet Table 5-1)

// Main / Status
#define CAP1206_REG_MAIN_CONTROL                0x00
#define CAP1206_REG_GENERAL_STATUS              0x02
#define CAP1206_REG_SENSOR_INPUT_STATUS         0x03
#define CAP1206_REG_NOISE_FLAG_STATUS           0x0A

// Delta count (read)
#define CAP1206_REG_SI1_DELTA_COUNT             0x10
#define CAP1206_REG_SI2_DELTA_COUNT             0x11
#define CAP1206_REG_SI3_DELTA_COUNT             0x12
#define CAP1206_REG_SI4_DELTA_COUNT             0x13
#define CAP1206_REG_SI5_DELTA_COUNT             0x14
#define CAP1206_REG_SI6_DELTA_COUNT             0x15

// Core configuration
#define CAP1206_REG_SENSITIVITY_CONTROL         0x1F
#define CAP1206_REG_CONFIGURATION               0x20
#define CAP1206_REG_SENSOR_INPUT_ENABLE         0x21
#define CAP1206_REG_SENSOR_INPUT_CONFIG         0x22
#define CAP1206_REG_SENSOR_INPUT_CONFIG2        0x23
#define CAP1206_REG_AVG_AND_SAMPLING_CONFIG     0x24
#define CAP1206_REG_CALIB_ACTIVATE_AND_STATUS   0x26
#define CAP1206_REG_INTERRUPT_ENABLE            0x27
#define CAP1206_REG_REPEAT_RATE_ENABLE          0x28

// Multiple Touch Pattern (MTP)
#define CAP1206_REG_MULTIPLE_TOUCH_CONFIG       0x2A
#define CAP1206_REG_MTP_CONFIG                  0x2B
#define CAP1206_REG_MTP_PATTERN                 0x2D

// Calibration / Recalibration status
#define CAP1206_REG_BASE_COUNT_OUT_OF_LIMIT     0x2E
#define CAP1206_REG_RECALIBRATION_CONFIG        0x2F

// Per-sensor thresholds (Active)
#define CAP1206_REG_SI1_THRESHOLD               0x30
#define CAP1206_REG_SI2_THRESHOLD               0x31
#define CAP1206_REG_SI3_THRESHOLD               0x32
#define CAP1206_REG_SI4_THRESHOLD               0x33
#define CAP1206_REG_SI5_THRESHOLD               0x34
#define CAP1206_REG_SI6_THRESHOLD               0x35

// Noise threshold
#define CAP1206_REG_SENSOR_INPUT_NOISE_THRESH   0x38

// Standby configuration
#define CAP1206_REG_STANDBY_CHANNEL             0x40
#define CAP1206_REG_STANDBY_CONFIG              0x41
#define CAP1206_REG_STANDBY_SENSITIVITY         0x42
#define CAP1206_REG_STANDBY_THRESHOLD           0x43
#define CAP1206_REG_CONFIGURATION2              0x44

// Base count (read)
#define CAP1206_REG_SI1_BASE_COUNT              0x50
#define CAP1206_REG_SI2_BASE_COUNT              0x51
#define CAP1206_REG_SI3_BASE_COUNT              0x52
#define CAP1206_REG_SI4_BASE_COUNT              0x53
#define CAP1206_REG_SI5_BASE_COUNT              0x54
#define CAP1206_REG_SI6_BASE_COUNT              0x55

// Power button feature
#define CAP1206_REG_POWER_BUTTON                0x60
#define CAP1206_REG_POWER_BUTTON_CONFIG         0x61

// Sensor input calibration (read)
#define CAP1206_REG_SI1_CALIBRATION             0xB1
#define CAP1206_REG_SI2_CALIBRATION             0xB2
#define CAP1206_REG_SI3_CALIBRATION             0xB3
#define CAP1206_REG_SI4_CALIBRATION             0xB4
#define CAP1206_REG_SI5_CALIBRATION             0xB5
#define CAP1206_REG_SI6_CALIBRATION             0xB6
#define CAP1206_REG_CALIBRATION_LSB1            0xB9  // CS1-CS4 LSBs
#define CAP1206_REG_CALIBRATION_LSB2            0xBA  // CS5-CS6 LSBs

// Identification (read)
#define CAP1206_REG_PRODUCT_ID                  0xFD
#define CAP1206_REG_MANUFACTURER_ID             0xFE
#define CAP1206_REG_REVISION                    0xFF


#define I2CADD (0x28u) << 1

static HAL_StatusTypeDef cap_write(uint16_t memadd, uint8_t data){
	return HAL_I2C_Mem_Write(&hi2c2, I2CADD, memadd, I2C_MEMADD_SIZE_8BIT,	&data,	1, 100);
}
static HAL_StatusTypeDef cap_read(uint16_t memadd, uint8_t *data){
	return HAL_I2C_Mem_Read(&hi2c2, I2CADD, memadd, I2C_MEMADD_SIZE_8BIT,	data,	1, 100);
}

void cap_test1(){
	cap_test3();
	while (HAL_GetTick() < 30000){
		HAL_Delay(1000);

		}
	}

void cap_test2(){
	uint8_t data;
	cap_read(CAP1206_REG_GENERAL_STATUS,&data);
	printstr("\n\rStatus: ");
	uart_print_u32((uint32_t)data);
	cap_read(CAP1206_REG_MAIN_CONTROL,&data);
	printstr("\n\rInitial Value: ");
	uart_print_u32((uint32_t)data);
	uint8_t data1 = 0b00100000;
	cap_write(CAP1206_REG_MAIN_CONTROL, data1);
	cap_read(CAP1206_REG_MAIN_CONTROL,&data);
	printstr("\n\r After write (0b00100000): ");
	uart_print_u32((uint32_t)data);
	data1 = 0b00010000;
	cap_write(CAP1206_REG_MAIN_CONTROL, data1);
	cap_read(CAP1206_REG_MAIN_CONTROL,&data);
	printstr("\n\r After write (0b00010000): ");
	uart_print_u32((uint32_t)data);
}

void cap_test3(){
	cap_init();
	cap_setmode(1);
}

void cap_init(){
	cap_write(CAP1206_REG_MAIN_CONTROL, 0b00000000);
	cap_write(CAP1206_REG_SENSITIVITY_CONTROL, 0b0010000); //[x|DSENSE[2:0]|BASESHIFT[3:0]] DSENSE: 111 - least sensitive1x, 000-most sensitive128x


}

void cap_setmode(uint8_t mode){
	switch (mode){
	case 1: //"Active"
		cap_write(CAP1206_REG_SENSOR_INPUT_ENABLE, 0b00100000); //ENABLE CS6, DISBLE OTHERS
		cap_write(CAP1206_REG_SENSOR_INPUT_CONFIG, 0b00100000);// [MX_DUR{3:0}|RPT_RAAATE[3:0]}] 560-11200 & 35-560 (35ms inc)

		break;
	default:
		break;
	}
}


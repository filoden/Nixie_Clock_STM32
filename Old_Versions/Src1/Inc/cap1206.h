/*
 * cap1206.h
 *
 *  Created on: Jan 12, 2026
 *      Author: Apath
 */

#ifndef INC_CAP1206_H_
#define INC_CAP1206_H_
#include <stdint.h>
// calibrates on startup - contact during this period may result in issues
// calibration can be forced
// automatic recal over time
// stuck button handling available


static HAL_StatusTypeDef cap_write(uint16_t memadd, uint8_t data);
static HAL_StatusTypeDef cap_read(uint16_t memadd, uint8_t *data);

void cap_test1();

void cap_test2();
void cap_test3();
void cap_init();
void cap_setmode(uint8_t mode);
#endif /* INC_CAP1206_H_ */

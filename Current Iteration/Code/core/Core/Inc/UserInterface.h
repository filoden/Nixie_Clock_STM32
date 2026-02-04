/*
 * UserInterface.h
 *
 *  Created on: Jan 31, 2026
 *      Author: Apath
 */

#ifndef INC_USERINTERFACE_H_
#define INC_USERINTERFACE_H_

#include <stdint.h>

extern volatile uint8_t USER_INT_REG0;
extern volatile uint8_t USER_INT_REG1;
extern volatile uint8_t USER_INT_REG2;
extern volatile uint8_t USER_INT_REG3;
extern volatile uint8_t USER_INT_REG4;
extern volatile uint8_t USER_INT_REG5;
extern volatile uint8_t USER_INT_REG6;
extern volatile uint8_t USER_INT_REG7;
extern volatile uint8_t FAILURE_CODE;
extern uint8_t AL_CTRL_REG; //[ AL1 ON | AL2 ON | x | x | x | x | x | x | ]
extern volatile uint32_t LAST_ROTATION;

void user_interaction_mode();
void playsound(uint16_t file);
void printmode(uint8_t mode);
void nixieDisplay(uint8_t text0, uint8_t text1, uint8_t option);
void addMinuteU8(uint8_t *var, uint8_t add);
void addHourU8(uint8_t *var, uint8_t add);
void addClampU8(uint8_t *var, uint8_t add, uint8_t clamp, uint8_t inc);
void setMode(uint8_t *setmode, char* str_enter, char* str_leave, uint8_t* perm1, uint8_t* perm2, uint8_t* temp1, uint8_t* temp2);

#endif /* INC_USERINTERFACE_H_ */

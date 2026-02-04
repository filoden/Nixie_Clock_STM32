/*
 * UserInterface.c
 *
 *  Created on: Jan 31, 2026
 *      Author: Apath
 */
#include "UserInterface.h"
#include <stdbool.h>
#include "debugging.h"
#include "stm32g0xx_hal.h"

#ifndef INC_DEBUGGING_H_
void printstr(char str[]){
	return;
}
#endif

#define UI_TIME_LIMIT 30 //User interface awake mode timeout in seconds

enum menu_select{set_time, set_alarm1, set_alarm2, set_brightness, set_volume, standard};

enum location{REG0, REG1, REG2, REG3, REG4, REG5, REG6, REG7};

#define BR_0 	2
#define BR_1 	1
#define VOL_0 2
#define VOL_1 1
#define KNOB_A	0x1
#define KNOB_A_CC 0x2
#define KNOB_B 4
#define KNOB_B_CC 0x8
#define KNOB_C 0x16
#define KNOB_C_CC 0x32


volatile uint8_t USER_INT_REG0 = 0;
volatile uint8_t USER_INT_REG1 = 0;
volatile uint8_t USER_INT_REG2 = 0;
volatile uint8_t USER_INT_REG3 = 0;
volatile uint8_t USER_INT_REG4 = 0;
volatile uint8_t USER_INT_REG5 = 0;
volatile uint8_t USER_INT_REG6 = 0;
volatile uint8_t USER_INT_REG7 = 0;
volatile uint8_t FAILURE_CODE = 0;
uint8_t AL_CTRL_REG = 0; //[ AL1 ON | AL2 ON | x | x | x | x | x | x | ]
volatile uint32_t LAST_ROTATION = 0;

struct UI_set{
	uint8_t brightness;
	uint8_t volume;
	uint8_t al1_m;
	uint8_t al2_m;
	uint8_t clock_m;
	uint8_t al1_h;
	uint8_t al2_h;
	uint8_t clock_h;
};





void user_interaction_mode(){
	struct UI_set settings = {};
	struct UI_set perms = {};
	printstr("Awakened.\n");
	uint8_t set_mode = 0;
	uint32_t timelimit = HAL_GetTick()+UI_TIME_LIMIT*1000;

	uint8_t cs = set_time;
	uint8_t intvar = 0;

	uint32_t time_step = HAL_GetTick();
	uint32_t curr_time = HAL_GetTick();
	while (HAL_GetTick() < timelimit){

		if (curr_time >= time_step){
			printstr("\n");
			time_step+=10000;
			}
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
			if (intvar & 0b1){ // Knob A rotating direction 1 GPIO-A0
				cs++;
				if (cs > set_volume){
					cs = set_time;
				}
				playsound(cs);
				printmode(cs);
				//printstr("\r\n next mode");
				set_mode = false;
			}
			else if (intvar & 0b10){ // Knob A rotating direction 2 GPIO-A3
				if (cs == set_time){
					cs = set_volume;
				}
				else {
					cs--;
				}
				playsound(cs);
				printmode(cs);
				//printstr("\r\n prev mode");
				set_mode = false;
			}

			else if ((intvar & 0b100) || (intvar & 0b1000)){ // Knob B (hours) rotation Handling
				intvar = ( intvar & 0b1100 );
				if (set_mode){
					switch (cs){
					case set_time :
						//printstr("times a movin");
						switch (intvar){
						case 0b100 :
							addHourU8(&settings.clock_h, 1);
						case 0b1000 :
							addHourU8(&settings.clock_h, 0);
							break;
						}
						nixieDisplay(settings.clock_h, settings.clock_m, 0);
						break;
					case set_alarm1 :
						//printstr("setting al1");
						switch (intvar){
						case 0b100 :
							addHourU8(&settings.al1_h, 1);
							break;

						case 0b1000 :
							addHourU8(&settings.al1_h, 0);
							break;
						}
						nixieDisplay(settings.al1_h, settings.al1_m, 0);
						break;
					case set_alarm2 :
						//printstr("setting al2");
						switch (intvar){
						case 0b100 :
							addHourU8(&settings.al2_h, 1);
							break;
						case 0b1000 :
							addHourU8(&settings.al2_h, 0);
							break;
						}
						nixieDisplay(settings.al2_h, settings.al2_m, 0);
						break;
					case set_volume :
						//printstr("setting volume");
						switch (intvar){
						case 0b100 :
							addClampU8(&settings.volume, (uint8_t)1, (uint8_t)99, VOL_0);
							break;
						case 0b1000 :
							addClampU8(&settings.volume, (uint8_t)0, (uint8_t)99, VOL_0);
							break;
						}
						nixieDisplay(settings.volume, 0, 0);
						break;
					case set_brightness :
						//printstr("setting brightness");
						switch (intvar){
						case 0b100 :
							addClampU8(&settings.brightness, (uint8_t)1, (uint8_t)99, BR_0);
							break;
						case 0b1000 :
							addClampU8(&settings.brightness, (uint8_t)0, (uint8_t)99, BR_0);
							break;
						}
						nixieDisplay(settings.brightness, 0, 0);
						break;
					default :
						printstr("\n\rundefined knob B/C rotation, no-op.");
						break;
					}
				}

				timelimit = HAL_GetTick()+UI_TIME_LIMIT*1000;
				intvar = 0;
				continue;
			}

			else if ((intvar & 0b10000) || (intvar & 0b100000)){ // Knob C (minutes) rotation Handling
				intvar = ( intvar & 0b110000 );
				if (set_mode){
					switch (cs){
					case set_time :
						//printstr("times a movin");
						switch (intvar){
						case 0b100 :
							addMinuteU8(&settings.clock_m, 1);
							break;
						case 0b1000 :
							addMinuteU8(&settings.clock_m, 0);
							break;
						}
						nixieDisplay(settings.clock_h, settings.clock_m, 0);
						break;
					case set_alarm1 :
						//printstr("setting al1");
						switch (intvar){
							case 0b100 :
							addMinuteU8(&settings.al1_m, 1);
							break;
						case 0b1000 :
							addMinuteU8(&settings.al1_m, 0);
							break;
						}
						nixieDisplay(settings.al1_h, settings.al1_m, 0);
						break;
					case set_alarm2 :
						//printstr("setting al2");
						switch (intvar){
						case 0b100 :
							addMinuteU8(&settings.al2_m, 1);
							break;
						case 0b1000 :
							addMinuteU8(&settings.al2_m, 0);
							break;
						}
						nixieDisplay(settings.al2_h, settings.al2_m, 0);
						break;
					case set_volume :
						//printstr("setting volume");
						switch (intvar){
						case 0b100 :
							addClampU8(&settings.volume, (uint8_t)1, (uint8_t)99, VOL_1);
							break;
						case 0b1000 :
							addClampU8(&settings.volume, (uint8_t)0, (uint8_t)99, VOL_1);
							break;
						}
						nixieDisplay(settings.volume, 0, 0);
						break;
					case set_brightness :
						//printstr("setting brightness");
						switch (intvar){
						case 0b100 :
							addClampU8(&settings.brightness, (uint8_t)1, (uint8_t)99, BR_1);
							break;
						case 0b1000 :
							addClampU8(&settings.brightness, (uint8_t)0, (uint8_t)99, BR_1);
							break;
						}
						nixieDisplay(settings.brightness, 0, 0);
						break;
					default :
						printstr("\n\rundefined knob B/C rotation, no-op.");
						break;
					}
				}

				timelimit = HAL_GetTick()+UI_TIME_LIMIT*1000;
				intvar = 0;
				continue;
			}
			else if (intvar & 0b01000000){ // switch is pressed on menu selection knob GPIO-C6
				switch (cs){
				case set_time :
					setMode(&set_mode, "\r\n Enter time set mode", "\r\n Time set to: \n", &perms.clock_h, &perms.clock_m, &settings.clock_h, &settings.clock_m);
					break;
				case set_alarm1 :
					setMode(&set_mode, "\r\n enter al1 set mode", "\r\n AL1 set to: \n", &perms.al1_h, &perms.al1_m, &settings.al1_h, &settings.al1_m);
					break;
				case set_alarm2 :
					setMode(&set_mode, "\r\n Enter al2 set mode", "\r\n AL2 set to: \n", &perms.al2_h, &perms.al2_m, &settings.al2_h, &settings.al2_m);
					break;
				case set_volume :
					setMode(&set_mode, "\r\n Enter volume set mode", "\r\n Volume set to: \n", &perms.volume, &perms.volume, &settings.volume, &settings.volume);\
					break;
				case set_brightness :
					setMode(&set_mode, "\r\n Enter brightness set mode", "\r\n Brightness set to: \n", &perms.brightness, &perms.brightness, &settings.brightness, &settings.brightness);
					break;
				}
			}
			timelimit = HAL_GetTick()+UI_TIME_LIMIT*1000;
			intvar = 0;
		}
	}
printstr("\r\n leaving UI mode...");
return;
}

void playsound(uint16_t file){ // sound list in order currently: set_time, set_alarm1, set_alarm2, toggle_alarm1, toggle_alarm2
	return;
}

void printmode(uint8_t mode){
	switch (mode){
		case (set_time):
			printstr("\n\rset time:");
			break;
		case (set_alarm1):
			printstr("\n\rset alarm1:");
			break;
		case (set_alarm2):
			printstr("\n\rset alarm2:");
			break;
		case (set_brightness):
			printstr("\n\rset brightness:");
			break;
		case (set_volume):
			printstr("\n\rset volume:");
			break;
	}
	return;
}

void nixieDisplay(uint8_t text0, uint8_t text1, uint8_t option){
	switch (option){
	case 0: // setting mode
		printstr("\r");
		uart_print_u32((uint32_t) text0);
		printstr(":");
		uart_print_u32((uint32_t) text1);
		printstr("   ");
	case standard:
		break;
	}
	return;
}

void HV_output(uint16_t text, uint8_t dimming){return;}

void addMinuteU8(uint8_t *var, uint8_t add){ // wrapper for adding mintes second argument is 1 for 'add' and 0 for 'subtract'
	if ( (*var) < 60){
		switch (add){
		case 1:
			switch (*var){
			case 59:
				*var = 0u;
				return;
			default :
				*var = *var + 1u;
				return;
			}
			break;
		case 0:
			switch (*var){
			case 0:
				*var = 59u;
				return;
			default :
				*var = *var - 1u;
				return;
			}
			break;
		default :
			printstr("\n\rerror, bad call to add clamp");
			FAILURE_CODE = 4;
			return;
		}
		return;
	}
	else{
		printstr("\n\rerror, minutes corrupted, exceeded 59");
		FAILURE_CODE = 3;
		return;
	}
}

void addHourU8(uint8_t *var, uint8_t add){ // wrapper for adding hours second argument is 1 for 'add' and 0 for 'subtract'
	if ( (*var) < 60){
		switch (add){
		case 1:
			switch (*var){
			case 23:
				*var = 0u;
				return;
			default :
				*var = *var + 1u;
				return;
			}
			break;
		case 0:
			switch (*var){
			case 0:
				*var = 23u;
				return;
			default :
				*var = *var - 1u;
				return;
			}
			break;
		default :
			printstr("\n\rerror, bad call to add clamp");
			FAILURE_CODE = 4;
			return;
		}
		return;
	}
	else{
		printstr("\n\rerror, hour corrupted, exceeded 23");
		FAILURE_CODE = 3;
		return;
	}
}

void addClampU8(uint8_t *var, uint8_t add, uint8_t clamp, uint8_t inc){ // wrapper for adding around a clmap second argument is 1 for 'add' and 0 for 'subtract'
	if ( (*var) <= clamp){
		switch (add){
		case 1:
			*var = (*var < clamp - inc) ? *var + inc : (*var+inc) % inc;
			return;
		case 0:
			*var = (*var >= inc) ? *var - inc : clamp + *var - inc +1;
			return;
		default :
			printstr("\n\rerror, bad call to add clamp");
			FAILURE_CODE = 4;
			return;
		}
		return;
	}
	else{
		printstr("\n\r error, clamp corrupted, exceeded inc");
		*var = 1;
		FAILURE_CODE = 3;
		return;
	}
}

void setMode(uint8_t *setmode, char* str_enter, char* str_leave, uint8_t* perm1, uint8_t* perm2, uint8_t* temp1, uint8_t* temp2){
	if (*setmode){
		*setmode = false;
		printstr(str_leave);
		uart_print_u32((uint32_t)(*temp1));
		uart_print_u32((uint32_t)(*temp2));
		*perm1 = *temp1;
		*perm2 = *temp2;
	}
	else{
		*setmode = true;
		printstr(str_enter);
		*temp1 = *perm1;
		*temp2 = *perm2;
	}
	return;
}

/*
 * UserInterface.c
 *
 *  Created on: Jan 31, 2026
 *      Author: Apath
 */
#include "UserInterface.h"
#include "audio.h"
#include <stdbool.h>
#include "debugging.h"
#include "stm32g0xx_hal.h"

#ifndef INC_DEBUGGING_H_
void printstr(char str[]){
	return;
}
#endif

#define UI_TIME_LIMIT 30 //User interface awake mode timeout in seconds
#define CLOCK 0
#define USER_SONG_OFFSET 40
#define SONGCLAMP (40 + counted_user_songs + 1)

enum temp {set_time, set_alarm1, set_alarm2, set_brightness, set_volume};

enum voiceTracks {
	AL1_ON = 0,
	AL1_OFF,
	AL2_ON,
	AL2_OFF,
	SET_TIME,
	SET_AL1,
	SET_AL2,
	SET_BRIGHTNESS,
	SET_VOL,
	SET_AL_TIME,
	SET_AL_SONG,
	SET_AL_DAYS,
	SET_AL_ON,
	SET_AL_OFF,
	SET_RETURN_MAIN,
	EDITING,
	ENTERING_ALARM_MENU,
	MAIN_MENU,
	CONFIRMED
};

enum soundTracks {
	AFFIRM = 30,
	INC,
	DEC
};

enum state {
    mm_set_time,
    mm_set_alarm1,
    mm_set_alarm2,
    mm_set_brightness,
    mm_set_volume,

    al_set_time,
    al_set_song,
    al_set_days,
    al_set_on_off,
    al_return_main,

    mm_set_time_edit,
    mm_set_brightness_edit,
    mm_set_volume_edit,

    al_set_time_edit,
    al_set_song_edit,
    al_set_days_edit,
    al_set_on_off_edit
};
enum location{REG0, REG1, REG2, REG3, REG4, REG5, REG6, REG7};;

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

volatile uint32_t LAST_ROTATION = 0;

struct UI_set {
    uint8_t brightness;
    uint8_t volume;

    uint8_t al1_m;
    uint8_t al2_m;
    uint8_t clock_m;

    uint8_t al1_h;
    uint8_t al2_h;
    uint8_t clock_h;

    uint8_t al1_song;
    uint8_t al2_song;

    uint8_t al1_days;	// [ X | S | M | T | W | Th | F | Sa ]
    uint8_t al2_days;  	// [ X | S | M | T | W | Th | F | Sa ]

    uint8_t al1_on;
    uint8_t al2_on;
};

struct UI_set temp_settings = {};
struct UI_set perm_settings = {};

static void knob_a_c();
static void knob_a_cc();
static void knob_b_c();
static void knob_b_cc();
static void knob_c_c();
static void knob_c_cc();
static void knob_a_pb();
static void incVolume();
static void decVolume();
static void incBrightness();
static void decBrightness();
static void incSong();
static void decSong();
static void incDays();
static void decDays();
// static void setResetal();
static void nop();
static void incMinutes(uint8_t type);
static void decMinutes(uint8_t type);
static void incHours(uint8_t type);
static void decHours(uint8_t type);
static void playVoice(uint8_t track);
static void show_perm_time();
uint8_t al = 0;

uint8_t intvar = 0;

int current_selection;
uint8_t set_mode;

void user_interaction_mode(){
	printstr("Awakened.\n");
	uint32_t timelimit = HAL_GetTick()+UI_TIME_LIMIT*1000;
	uint8_t state = mm_set_time;
	uint8_t intvar = 0;

	while (HAL_GetTick() < timelimit){ // while UX is not timed-out (from lack of user interaction)


		// Check the USER_INT_REGs for any contents
		// registers serve as a FIFO. Each register should contain exactly 1 asserted bit to indicate only one of the seven possible signals
		// USER_INT_REGx = [ x | PUSHBUTTON | Main Knob - C | Main Knob - CC | Hour Knob - C | Hour Knob - CC | Minute Knob - C | Minute Knob CC  ]
		// In testing only REG7 and sometimes REG6 were ever in use, but hey, maybe you've got quicker hands

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

		// The interrupt FIFO has now been checked. If there is an interrupt, it will be serviced in the following
		if (intvar != 0){

			switch (intvar){
			case (0b10000001):
					knob_a_c(&state);
					break;
			case (0b10000010):
					knob_a_cc(&state);
					break;
			case (0b10000100):
					knob_b_c(&state);
					break;
			case (0b10001000):
					knob_b_cc(&state);
					break;
			case (0b10010000):
					knob_c_c(&state);
					break;
			case (0b10100000):
					knob_c_cc(&state);
					break;
			case (0b11000000):
					knob_a_pb(&state);
					break;
			default:
					printstr("unknown interrupt vector handed to user interaction handler (Userinterface.c: 155.\n\r");
					}
			// erase the interrupt as it has now been serviced
			intvar = 0;
			// Refresh the UX timeout
			timelimit = HAL_GetTick()+UI_TIME_LIMIT*1000;
		}
	}
printstr("\r\n leaving UI mode...");
return;
}


void printmode(uint8_t mode)
{
    switch ((enum state)mode) {
        case mm_set_time:
            printstr("\n\rset time:");
            break;

        case mm_set_alarm1:
            printstr("\n\rset alarm1:");
            break;

        case mm_set_alarm2:
            printstr("\n\rset alarm2:");
            break;

        case mm_set_brightness:
            printstr("\n\rset brightness:");
            break;

        case mm_set_volume:
            printstr("\n\rset volume:");
            break;

        case al_set_time:
            if (al == 1) {
                printstr("\n\rset alarm 1 time:");
            }
            else if (al == 2) {
                printstr("\n\rset alarm 2 time:");
            }
            else {
                printstr("\n\rset alarm time:");
            }
            break;

        case al_set_song:
            if (al == 1) {
                printstr("\n\rset alarm 1 song:");
            }
            else if (al == 2) {
                printstr("\n\rset alarm 2 song:");
            }
            else {
                printstr("\n\rset alarm song:");
            }
            break;

        case al_set_days:
            if (al == 1) {
                printstr("\n\rset alarm 1 days:");
            }
            else if (al == 2) {
                printstr("\n\rset alarm 2 days:");
            }
            else {
                printstr("\n\rset alarm days:");
            }
            break;

        case al_set_on_off:
            if (al == 1) {
                printstr("\n\rset alarm 1 on/off:");
            }
            else if (al == 2) {
                printstr("\n\rset alarm 2 on/off:");
            }
            else {
                printstr("\n\rset alarm on/off:");
            }
            break;

        case al_return_main:
            if (al == 1) {
                printstr("\n\rreturn to main menu from alarm 1:");
            }
            else if (al == 2) {
                printstr("\n\rreturn to main menu from alarm 2:");
            }
            else {
                printstr("\n\rreturn to main menu:");
            }
            break;

        case mm_set_time_edit:
            printstr("\n\redit time:");
            break;

        case mm_set_brightness_edit:
            printstr("\n\redit brightness:");
            break;

        case mm_set_volume_edit:
            printstr("\n\redit volume:");
            break;

        case al_set_time_edit:
            if (al == 1) {
                printstr("\n\redit alarm 1 time:");
            }
            else if (al == 2) {
                printstr("\n\redit alarm 2 time:");
            }
            else {
                printstr("\n\redit alarm time:");
            }
            break;

        case al_set_song_edit:
            if (al == 1) {
                printstr("\n\redit alarm 1 song:");
            }
            else if (al == 2) {
                printstr("\n\redit alarm 2 song:");
            }
            else {
                printstr("\n\redit alarm song:");
            }
            break;

        case al_set_days_edit:
            if (al == 1) {
                printstr("\n\redit alarm 1 days:");
            }
            else if (al == 2) {
                printstr("\n\redit alarm 2 days:");
            }
            else {
                printstr("\n\redit alarm days:");
            }
            break;

        case al_set_on_off_edit:
            if (al == 1) {
                printstr("\n\redit alarm 1 on/off:");
            }
            else if (al == 2) {
                printstr("\n\redit alarm 2 on/off:");
            }
            else {
                printstr("\n\redit alarm on/off:");
            }
            break;

        default:
            printstr("\n\rError: unknown mode");
            break;
    }
}

void nixieDisplay(uint8_t text0, uint8_t text1, uint8_t option){
	switch (option){
	case 0: // setting mode
		printstr("\r");
		uart_print_u32((uint32_t) text0);
		printstr(":");
		uart_print_u32((uint32_t) text1);
		printstr("   ");
	}
	return;
}

void HV_output(uint16_t text, uint8_t dimming){dimming++;text++;return;}

void addClampU8(uint8_t *var, uint8_t add, uint8_t clamp, uint8_t inc){ // wrapper for adding around a clamp second argument is 1 for 'add' and 0 for 'subtract'
	//@param pointer to variable to be incremented
	//@param 1 for adding 0 for subtracting
	//@param MAX value for variable + 1
	//@param size of increment
    if (var == NULL) {
        printstr("\n\rError: NULL pointer passed to addClampU8.");
        return;
    }

    if ((clamp == 0u) || (inc == 0u)) {
        printstr("\n\rError: invalid clamp or increment.");
        return;
    }

    if (*var >= clamp) {
        printstr("\n\rError: clamped variable was out of range.");
        *var = 0u;
        return;
    }
	switch (add){
		case 1:
			*var = (*var < clamp - inc) ? *var + inc : (*var+inc) % inc;
			return;
		case 0:
			*var = (*var >= inc) ? *var - inc : clamp + *var - inc;
			return;
		default :
			printstr("\n\rerror, bad call to add clamp");
			FAILURE_CODE = 4;
			return;
	}
	return;
}

void addLimitU8(uint8_t *var, uint8_t add, uint8_t clamp, uint8_t inc, uint8_t min){
	addClampU8(var, add, clamp, inc);
	if ( (*var < min) && (add == 1) ){
		*var = min;
	}
	if ( (*var < min) && (add == 0) ){
		*var = clamp - 1;
	}
	return;
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

static void knob_a_c(uint8_t *state) // Knob A rotating clockwise
{
    uint8_t ns = *state;

    switch ((enum state)(*state)) {
        case mm_set_time:
            ns = mm_set_alarm1;
            playVoice(SET_AL1);
            printmode(ns);
            break;

        case mm_set_alarm1:
            ns = mm_set_alarm2;
            playVoice(SET_AL2);
            printmode(ns);
            break;

        case mm_set_alarm2:
            ns = mm_set_brightness;
            playVoice(SET_BRIGHTNESS);
            printmode(ns);
            break;

        case mm_set_brightness:
            ns = mm_set_volume;
            playVoice(SET_VOL);
            printmode(ns);
            break;

        case mm_set_volume:
            ns = mm_set_time;
            playVoice(SET_TIME);
            printmode(ns);
            break;
// NOT DONE
        case al_set_time:
            ns = al_set_song;
            playVoice(SET_AL_SONG);
            printmode(ns);
            break;

        case al_set_song:
            ns = al_set_days;
            playVoice(SET_AL_DAYS);
            printmode(ns);
            break;

        case al_set_days:
            ns = al_set_on_off;
            uint8_t al_state;
            if (al == 1){
            	al_state = perm_settings.al1_on;
            }
            else if (al == 2){
            	al_state = perm_settings.al2_on;
            }
            else {
            	printstr("Invalid al value.\n\r");
            }
            if (al_state){
            	playVoice(SET_AL_OFF);
            }
            else {
            	playVoice(SET_AL_ON);
            }
            printmode(ns);
            break;

        case al_set_on_off:
            ns = al_return_main;
            playVoice(SET_RETURN_MAIN);
            printmode(ns);
            break;

        case al_return_main:
            ns = al_set_time;
            playVoice(SET_AL_TIME);
            printmode(ns);
            break;

        case mm_set_time_edit:
            playSound(INC);
            incMinutes(CLOCK);
            break;

        case mm_set_brightness_edit:
            playSound(INC);
            incBrightness();
            break;

        case mm_set_volume_edit:
            playSound(INC);
            incVolume();
            break;

        case al_set_time_edit:
            playSound(INC);
            incMinutes(al);
            break;

        case al_set_song_edit:
            playSound(INC);
            incSong(al);
            break;

        case al_set_days_edit:
            playSound(INC);
            incDays(al);
            break;

        case al_set_on_off_edit:
            nop();
            break;

        default:
            printstr("Error, Invalid state unknown");
            ns = mm_set_time;
            playVoice(SET_TIME);
            printmode(ns);
            break;
    }

    *state = ns;
}


static void knob_a_cc(uint8_t *state) // Knob A rotating counterclockwise
{
    uint8_t ns = *state;

    switch ((enum state)(*state)) {
        case mm_set_time:
            ns = mm_set_volume;
            playVoice(SET_VOL);
            printmode(ns);
            break;

        case mm_set_alarm1:
            ns = mm_set_time;
            playVoice(SET_TIME);
            printmode(ns);
            break;

        case mm_set_alarm2:
            ns = mm_set_alarm1;
            playVoice(SET_AL1);
            printmode(ns);
            break;

        case mm_set_brightness:
            ns = mm_set_alarm2;
            playVoice(SET_AL2);
            printmode(ns);
            break;

        case mm_set_volume:
            ns = mm_set_brightness;
            playVoice(SET_BRIGHTNESS);
            printmode(ns);
            break;

        case al_set_time:
            ns = al_return_main;
            playVoice(SET_RETURN_MAIN);
            printmode(ns);
            break;

        case al_set_song:
            ns = al_set_time;
            playVoice(SET_AL_TIME);
            printmode(ns);
            break;

        case al_set_days:
            ns = al_set_song;
            playVoice(SET_AL_SONG);
            printmode(ns);
            break;

        case al_set_on_off:
            ns = al_set_days;
            playVoice(SET_AL_DAYS);
            printmode(ns);
            break;

        case al_return_main:
            ns = al_set_on_off;
            ns = al_set_on_off;
			uint8_t al_state;
			if (al == 1){
				al_state = perm_settings.al1_on;
			}
			else if (al == 2){
				al_state = perm_settings.al2_on;
			}
			else {
				printstr("Invalid al value.\n\r");
			}
			if (al_state){
				playVoice(SET_AL_OFF);
			}
			else {
				playVoice(SET_AL_ON);
			}
			printmode(ns);
			break;


        case mm_set_time_edit:
            playSound(DEC);
            decMinutes(CLOCK);
            break;

        case mm_set_brightness_edit:
            playSound(DEC);
            decBrightness();
            break;

        case mm_set_volume_edit:
            playSound(DEC);
            decVolume();
            break;

        case al_set_time_edit:
            playSound(DEC);
            decMinutes(al);
            break;

        case al_set_song_edit:
            playSound(DEC);
            decSong(al);
            break;

        case al_set_days_edit:
            playSound(DEC);
            decDays(al);
            break;

        case al_set_on_off_edit:
            playSound(DEC);
            nop();
            break;

        default:
            printstr("Error, Invalid state unknown");
            ns = mm_set_time;
            playVoice(SET_TIME);
            printmode(ns);
            break;
    }

    *state = ns;
}


static void knob_b_c(uint8_t *state) // Knob B clockwise
{
    uint8_t ns = *state;

    switch ((enum state)(*state)) {
        case mm_set_time:
        case mm_set_alarm1:
        case mm_set_alarm2:
        case mm_set_brightness:
        case mm_set_volume:
        case al_set_time:
        case al_set_song:
        case al_set_days:
        case al_set_on_off:
        case al_return_main:
            nop();
            break;

        case mm_set_time_edit:
            incHours(CLOCK);
            break;

        case mm_set_brightness_edit:
            incBrightness();
            break;

        case mm_set_volume_edit:
            incVolume();
            break;

        case al_set_time_edit:
            incHours(al);
            break;

        case al_set_song_edit:
            incSong(al);
            break;

        case al_set_days_edit:
            incDays(al);
            break;

        case al_set_on_off_edit:
            //setResetal(al);
        	nop();
            break;

        default:
            printstr("Error, Invalid state unknown");
            ns = mm_set_time;
            printmode(ns);
            break;
    }

    *state = ns;
}


static void knob_b_cc(uint8_t *state) // Knob B counterclockwise
{
    uint8_t ns = *state;

    switch ((enum state)(*state)) {
        case mm_set_time:
        case mm_set_alarm1:
        case mm_set_alarm2:
        case mm_set_brightness:
        case mm_set_volume:
        case al_set_time:
        case al_set_song:
        case al_set_days:
        case al_set_on_off:
        case al_return_main:
            nop();
            break;

        case mm_set_time_edit:
            decHours(CLOCK);
            break;

        case mm_set_brightness_edit:
            decBrightness();
            break;

        case mm_set_volume_edit:
            decVolume();
            break;

        case al_set_time_edit:
            decHours(al);
            break;

        case al_set_song_edit:
            decSong(al);
            break;

        case al_set_days_edit:
            decDays(al);
            break;

        case al_set_on_off_edit:
        	nop();
            break;

        default:
            printstr("Error, Invalid state unknown");
            ns = mm_set_time;
            printmode(ns);
            break;
    }

    *state = ns;
}


static void knob_c_c(uint8_t *state) // Knob C clockwise
{
    uint8_t ns = *state;

    switch ((enum state)(*state)) {
        case mm_set_time:
        case mm_set_alarm1:
        case mm_set_alarm2:
        case mm_set_brightness:
        case mm_set_volume:
        case al_set_time:
        case al_set_song:
        case al_set_days:
        case al_set_on_off:
        case al_return_main:
            nop();
            break;

        case mm_set_time_edit:
            incHours(CLOCK);
            break;

        case mm_set_brightness_edit:
            incBrightness();
            break;

        case mm_set_volume_edit:
            incVolume();
            break;

        case al_set_time_edit:
            incHours(al);
            break;

        case al_set_song_edit:
            incSong(al);
            break;

        case al_set_days_edit:
            incDays(al);
            break;

        case al_set_on_off_edit:
            //setResetal(al);
        	nop();
            break;

        default:
            printstr("Error, Invalid state unknown");
            ns = mm_set_time;
            printmode(ns);
            break;
    }

    *state = ns;
}


static void knob_c_cc(uint8_t *state) // Knob C counterclockwise
{
    uint8_t ns = *state;

    switch ((enum state)(*state)) {
        case mm_set_time:
        case mm_set_alarm1:
        case mm_set_alarm2:
        case mm_set_brightness:
        case mm_set_volume:
        case al_set_time:
        case al_set_song:
        case al_set_days:
        case al_set_on_off:
        case al_return_main:
            nop();
            break;

        case mm_set_time_edit:
            decHours(CLOCK);
            break;

        case mm_set_brightness_edit:
            decBrightness();
            break;

        case mm_set_volume_edit:
            decVolume();
            break;

        case al_set_time_edit:
            decHours(al);
            break;

        case al_set_song_edit:
            decSong(al);
            break;

        case al_set_days_edit:
            decDays(al);
            break;

        case al_set_on_off_edit:
            //setResetal(al);
        	nop();
            break;

        default:
            printstr("Error, Invalid state unknown");
            ns = mm_set_time;
            printmode(ns);
            break;
    }

    *state = ns;
}


static void knob_a_pb(uint8_t *state) // Knob A pushbutton: enter / commit / return
{
    uint8_t ns = *state;

    switch ((enum state)(*state)) {
        case mm_set_time:
            temp_settings = perm_settings;
            ns = mm_set_time_edit;
            playSound(AFFIRM);
            playVoice(EDITING);
            printmode(ns);
            nixieDisplay(temp_settings.clock_h, temp_settings.clock_m, 0);
            break;

        case mm_set_alarm1:
            temp_settings = perm_settings;
            al = 1;
            ns = al_set_time;
            playSound(AFFIRM);
            playVoice(ENTERING_ALARM_MENU);
            nixieDisplay(temp_settings.al1_h, temp_settings.al1_m, 0);
            printmode(ns);
            break;

        case mm_set_alarm2:
            temp_settings = perm_settings;
            al = 2;
            ns = al_set_time;
            playSound(AFFIRM);
            playVoice(ENTERING_ALARM_MENU);
            nixieDisplay(temp_settings.al2_h, temp_settings.al2_m, 0);
            printmode(ns);
            break;

        case mm_set_brightness:
            temp_settings = perm_settings;
            playSound(AFFIRM);
            playVoice(EDITING);
            ns = mm_set_brightness_edit;
            printmode(ns);
            nixieDisplay(temp_settings.brightness, 0, 0);
            break;
        case mm_set_volume:
            temp_settings = perm_settings;
            playSound(AFFIRM);
            playVoice(EDITING);
            ns = mm_set_volume_edit;
            printmode(ns);
            nixieDisplay(temp_settings.volume, 0, 0);
            break;

        case al_set_time:
            temp_settings = perm_settings;
            playSound(AFFIRM);
            playVoice(EDITING);
            ns = al_set_time_edit;
            printmode(ns);
            if (al == 1) {
                nixieDisplay(temp_settings.al1_h, temp_settings.al1_m, 0);
            }
            else if (al == 2) {
                nixieDisplay(temp_settings.al2_h, temp_settings.al2_m, 0);
            }
            else {
                printstr("Error, Invalid alarm selected");
            }
            break;

        case al_set_song:
            temp_settings = perm_settings;
            playSound(AFFIRM);
            playVoice(EDITING);
            ns = al_set_song_edit;
            printmode(ns);
            break;

        case al_set_days:
            temp_settings = perm_settings;
            playSound(AFFIRM);
            playVoice(EDITING);
            ns = al_set_days_edit;
            printmode(ns);
            break;

        case al_set_on_off: // User has selected to change the mode on an alarm from either on to off or off to on
            temp_settings = perm_settings;
        	if (al == 1){
                uint8_t temp = perm_settings.al1_on;
                if (temp == 1){ // al1 is on, set it off
                	perm_settings.al1_on = 0;

                	playVoice(AL1_OFF); // AL1 set off
                	printstr("AL1_OFF\n\r");
                }
                else if (temp == 0){ // al1 is off, set it on
                	perm_settings.al1_on = 1;

                	playVoice(AL1_ON); // AL1 set on
                	printstr("AL1_ON\n\r");
                }
                else{
                	printstr("Error, received undefined status in al_set_on_ff in knob_a_pb.\n\r");
                }
        	}
        	else if (al == 2){
                uint8_t temp = perm_settings.al2_on;
                if (temp == 1){ // al2 is on, set it off
                	perm_settings.al1_on = 0;

                	playVoice(AL2_OFF); // AL2 set off
                	printstr("AL2_OFF\n\r");
                }
                else if (temp == 0){ // al2 is off, set it on
                	perm_settings.al1_on = 1;

                	playVoice(AL2_ON); // AL2 set on
                	printstr("AL2_ON\n\r");
                }
                else{
                	printstr("Error, received undefined status in al_set_on_ff in knob_a_pb.\n\r");
                }
        	}
        	else{
        		printstr("Error, al set to uknonwn value.\n\r");
        	}
            break;

        case al_return_main:
        	ns = mm_set_time;
        	playSound(AFFIRM);
            playVoice(MAIN_MENU);
        	playVoice(SET_TIME);
            printmode(ns);
            break;

        case mm_set_time_edit:
            perm_settings.clock_h = temp_settings.clock_h;
            perm_settings.clock_m = temp_settings.clock_m;
            ns = mm_set_time;
            show_perm_time();
            playSound(AFFIRM); //Affirm
            playVoice(CONFIRMED);
            printmode(ns);
            break;

        case mm_set_brightness_edit:
            perm_settings.brightness = temp_settings.brightness;
            ns = mm_set_brightness;
            show_perm_time();
            playSound(AFFIRM); //Affirm
            playVoice(CONFIRMED);
            printmode(ns);
            break;

        case mm_set_volume_edit:
            perm_settings.volume = temp_settings.volume;
            ns = mm_set_volume;
            show_perm_time();
            playSound(AFFIRM); //Affirm
            playVoice(CONFIRMED);
            printmode(ns);
            break;

        case al_set_time_edit:
            if (al == 1) {
                perm_settings.al1_h = temp_settings.al1_h;
                perm_settings.al1_m = temp_settings.al1_m;
            }
            else if (al == 2) {
                perm_settings.al2_h = temp_settings.al2_h;
                perm_settings.al2_m = temp_settings.al2_m;
            }
            ns = al_set_time;
            show_perm_time();
            playSound(AFFIRM); //Affirm
            playVoice(CONFIRMED);
            printmode(ns);
            break;

        case al_set_song_edit:
            perm_settings = temp_settings;
            /*
             * Song is not currently in struct UI_set.
             * Commit song setting here if you add it later.
             */
            ns = al_set_song;
            playSound(AFFIRM); //Affirm
            playVoice(CONFIRMED);
            printmode(ns);
            break;

        case al_set_days_edit:
            perm_settings = temp_settings;
            /*
             * Days is not currently in struct UI_set.
             * Commit day setting here if you add it later.
             */
            ns = al_set_days;
            playSound(AFFIRM); //Affirm
            playVoice(CONFIRMED);
            printmode(ns);
            break;
        default:
            printstr("Error, Invalid state unknown");
            ns = mm_set_time;
            printmode(ns);
            break;
    }

    *state = ns;
}

#define USER_SONG_OFFSE        ((uint8_t)0)
#define SONG_MAX_PLUS_1 ((uint8_t)10)   // replace 10 with number of songs

#define DAYS_MIN        ((uint8_t)0)
#define DAYS_MAX_PLUS_1 ((uint8_t)128)  // 0-127 if using 7-bit day mask

static void incVolume()
{
    addClampU8(&temp_settings.volume, (uint8_t)1u, (uint8_t)100u, (uint8_t)VOL_0);
    nixieDisplay(temp_settings.volume, 0u, 0u);
    printstr("\rVolume: ");
    uart_print_u32(temp_settings.volume);
}

static void decVolume()
{
    addClampU8(&temp_settings.volume, (uint8_t)0u, (uint8_t)100u, (uint8_t)VOL_0);
    nixieDisplay(temp_settings.volume, 0u, 0u);
    printstr("\rVolume: ");
    uart_print_u32(temp_settings.volume);
}

static void incBrightness()
{
    addClampU8(&temp_settings.brightness, (uint8_t)1u, (uint8_t)100u, BR_0);
    nixieDisplay(temp_settings.brightness, 0u, 0u);
    printstr("\rBrightness: ");
    uart_print_u32(temp_settings.brightness);
}

static void decBrightness()
{
    addClampU8(&temp_settings.brightness, (uint8_t)0u, (uint8_t)100u, BR_0);
    nixieDisplay(temp_settings.brightness, 0u, 0u);
    printstr("\rBrightness: ");
    uart_print_u32(temp_settings.brightness);
}

static void incSong()
{

    if (al == 1) {
        addLimitU8(&temp_settings.al1_song, (uint8_t)1u, (uint8_t)SONGCLAMP, (uint8_t)1u, (uint8_t)USER_SONG_OFFSET);
        playSound(temp_settings.al1_song);
        printstr("\rAlarm 1 Song: ");
        uart_print_u32(temp_settings.al1_song);
    }
    else if (al == 2) {
        addLimitU8(&temp_settings.al2_song, (uint8_t)1u, (uint8_t)SONGCLAMP, (uint8_t)1u, (uint8_t)USER_SONG_OFFSET);
        playSound(temp_settings.al2_song);
        printstr("\rAlarm 2 Song: ");
        uart_print_u32(temp_settings.al2_song);
    }
    else {
        printstr("Error, Invalid alarm selected");
    }
}

static void decSong()
{
    if (al == 1) {
        addLimitU8(&temp_settings.al1_song, (uint8_t)0, SONGCLAMP, (uint8_t)1u, (uint8_t)USER_SONG_OFFSET);
        playSound(temp_settings.al1_song);
        printstr("\rAlarm 1 Song: ");
        uart_print_u32(temp_settings.al1_song);
    }
    else if (al == 2) {
        addLimitU8(&temp_settings.al2_song, (uint8_t)0, SONGCLAMP, (uint8_t)1u, (uint8_t)USER_SONG_OFFSET);
        playSound(temp_settings.al2_song);
        printstr("\rAlarm 2 Song: ");
        uart_print_u32(temp_settings.al2_song);
    }
    else {
        printstr("Error, Invalid alarm selected");
    }
}

static void incDays()
{
    if (al == 1) {
        addClampU8(&temp_settings.al1_days, (uint8_t)1, DAYS_MAX_PLUS_1, DAYS_MIN);

        printstr("\rAlarm 1 Days: ");
        uart_print_u32(temp_settings.al1_days);
    }
    else if (al == 2) {
        addClampU8(&temp_settings.al2_days, (uint8_t)1, DAYS_MAX_PLUS_1, DAYS_MIN);

        printstr("\rAlarm 2 Days: ");
        uart_print_u32(temp_settings.al2_days);
    }
    else {
        printstr("Error, Invalid alarm selected");
    }
}

static void decDays()
{
    if (al == 1) {
        addClampU8(&temp_settings.al1_days, (uint8_t)0, DAYS_MAX_PLUS_1, DAYS_MIN);

        printstr("\rAlarm 1 Days: ");
        uart_print_u32(temp_settings.al1_days);
    }
    else if (al == 2) {
        addClampU8(&temp_settings.al2_days, (uint8_t)0, DAYS_MAX_PLUS_1, DAYS_MIN);

        printstr("\rAlarm 2 Days: ");
        uart_print_u32(temp_settings.al2_days);
    }
    else {
        printstr("Error, Invalid alarm selected");
    }
}
/*
static void setResetal()
{
    if (al == 1) {
        temp_settings.al1_on = !temp_settings.al1_on;

        printstr("\rAlarm 1: ");
        if (temp_settings.al1_on) {
            printstr("ON");
        }
        else {
            printstr("OFF");
        }
    }
    else if (al == 2) {
        temp_settings.al2_on = !temp_settings.al2_on;

        printstr("\rAlarm 2: ");
        if (temp_settings.al2_on) {
            printstr("ON");
        }
        else {
            printstr("OFF");
        }
    }
    else {
        printstr("Error, Invalid alarm selected");
    }
}
*/

static void nop()
{
    printstr("NOP");
}

static void incMinutes(uint8_t type){
	switch (type){
	case (CLOCK):
			addClampU8(&temp_settings.clock_m, (uint8_t)1, (uint8_t)60, (uint8_t)1);
			break;
	case (1):
			addClampU8(&temp_settings.al1_m, (uint8_t)1, (uint8_t)60, (uint8_t)1);
			break;
	case (2):
			addClampU8(&temp_settings.al2_m, (uint8_t)1, (uint8_t)60, (uint8_t)1);
			break;
	}
	return;
}
static void decMinutes(uint8_t type){
	switch (type){
	case (CLOCK):
			addClampU8(&temp_settings.clock_m, (uint8_t)0, (uint8_t)60, (uint8_t)1);
			break;
	case (1):
			addClampU8(&temp_settings.al1_m, (uint8_t)0, (uint8_t)60, (uint8_t)1);
			break;
	case (2):
			addClampU8(&temp_settings.al2_m, (uint8_t)0, (uint8_t)60, (uint8_t)1);
			break;
	}
	return;
}
static void incHours(uint8_t type){
	switch (type){
	case (CLOCK):
			addClampU8(&temp_settings.clock_h, (uint8_t)1, (uint8_t)24, (uint8_t)1);
			break;
	case (1):
			addClampU8(&temp_settings.al1_h, (uint8_t)1, (uint8_t)24, (uint8_t)1);
			break;
	case (2):
			addClampU8(&temp_settings.al2_h, (uint8_t)1, (uint8_t)24, (uint8_t)1);
			break;
	}
	return;
}
static void decHours(uint8_t type){
	switch (type){
	case (CLOCK):
			addClampU8(&temp_settings.clock_h, (uint8_t)0, (uint8_t)24, (uint8_t)1);
			break;
	case (1):
			addClampU8(&temp_settings.al1_h, (uint8_t)0, (uint8_t)24, (uint8_t)1);
			break;
	case (2):
			addClampU8(&temp_settings.al2_h, (uint8_t)0, (uint8_t)24, (uint8_t)1);
			break;
	}
	return;
}


static void playVoice(uint8_t track){
	track++;
	return;
}


static void show_perm_time(){
	return;
}

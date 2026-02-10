/*
 * audio.c
 *
 *  Created on: Feb 8, 2026
 *      Author: Apath
 */

//#include "app_fatfs.h"
#include "audio.h"

#define RB_SIZE (32 * 1024)   // 32KB ring buffer (tune) must be a power of two
#define HRB_SIZE (RB_SIZE/2)
static uint8_t rb[RB_SIZE];
static volatile uint32_t rb_w = 0, rb_r = 0;
static volatile uint8_t STOPSONG = 0;

static inline uint32_t rb_count(void) {
  return (rb_w - rb_r) & (RB_SIZE - 1);
}

static inline uint32_t rb_space(void) {
  return (RB_SIZE - 1) - rb_count();
}

static uint8_t initsound(FATFS* fs, FIL * fil, fileData* fn, uint8_t song){
	uint8_t err = f_mount(fs, "0:", 1);
	switch (err){
	case (0):
			break;
	default:
		printstr("Playback Error. Error mounting to intitiate song");
		return 1;
	}
	err = f_open(fil, fn->name[song], FA_OPEN_EXISTING | FA_READ); // open sound in SD
	switch (err){
	case (0):
		break;
	default:
		printstr("Playback Error. Error opening to intitiate song");
		return 1;
	}
	err = f_lseek(fil, fn->fmt[song].datastart);					// move to start of data
	switch (err){
	case (0):
		break;
	default:
		printstr("Playback Error. Error seeking on song init");
		return 1;
	}
	UINT br;
	err = f_read(fil, rb, RB_SIZE, &br);
	switch (err){
	case (0):
		break;
	default:
		printstr("Playback Error. Error reading on intitiate song");
		return 1;
	}
	rb_w = br;
	return 0;
}



static inline uint8_t loadbuff(FIL * fil){
		UINT br1 = 0;
		UINT br2 = 0;
		uint32_t write = rb_w & (RB_SIZE - 1);
		uint8_t err1 = 0;
		uint8_t err2 = 0;
		if (write < HRB_SIZE){
		err1 = f_read(fil, &rb[write], HRB_SIZE, &br1);
		}
		else{
			err1 = f_read(fil, &rb[write], RB_SIZE - write, &br1);
			err2 = f_read(fil, rb, write - br1, &br2);
		}
		switch (err1 | err2){
		case (0):
			break;
		default:
			printstr("Playback Error. Error reading on loading buffer");
			return 1;
		}
		rb_w += br1 + br2;
		return 0;
}

void blockingPlaySound(fileData* fn, uint8_t song){
	FATFS fs;
	FIL fil;
	if (initsound(&fs, &fil, fn, song)){
		return;
	}
	while (!(STOPSONG)){
		if (rb_space() > HRB_SIZE) {
			if (loadbuff(&fil) != 0){
				f_mount(NULL, "0:", 0);
				return;
			}
		}
	}
}

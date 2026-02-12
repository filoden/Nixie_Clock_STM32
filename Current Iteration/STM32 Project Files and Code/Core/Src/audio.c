/*
 * audio.c
 *
 *  Created on: Feb 8, 2026
 *      Author: Apath
 */

//#include "app_fatfs.h"
#include "audio.h"
#include "debugging.h"

#define RB_SIZE (64 * 1024)   // 32KB ring buffer (tune) must be a power of two
#define RB_MASK (RB_SIZE - 1)// 32KB ring buffer (tune) must be a power of two
#define HRB_SIZE (RB_SIZE/2)
#define FR_PER_H (1024u * 4)
#define HW_PER_FR 2
#define HW_PER_H (FR_PER_H * HW_PER_FR)

static uint8_t rb[RB_SIZE];
static volatile uint32_t rb_w = 0, rb_r = 0;
static volatile uint8_t STOPSONG = 0;
volatile uint8_t I2SERR = 0;

extern I2S_HandleTypeDef hi2s2;
static uint16_t i2s_tx[HW_PER_H * 2u];  // 2 halves
static volatile uint8_t need_refill = 0;
static volatile uint8_t eof_seen = 0;
static uint8_t file_channels = 0;
static volatile uint16_t overwrite = 0;
static volatile uint16_t underrun = 0;
static uint32_t time_count[2][100] = {0};

static inline uint32_t rb_count(void) {
	/*static uint32_t overrun = 0;
		if(rb_w - rb_r > RB_SIZE){
			overrun++;
		}*/
  return (rb_w - rb_r) ;
}

static inline uint32_t rb_space(void) {
	/*static uint32_t overrun = 0;
	if(rb_w - rb_r > RB_SIZE){
		overrun++;
	}*/
  return (RB_SIZE  - rb_count() );
}

static inline int16_t rb_pop_i16(void){
	if (rb_count() < 2u) return 0;
	uint32_t i0 = rb_r & RB_MASK;
	uint8_t d0 = rb[i0];
	rb_r++;
	i0 = rb_r & RB_MASK;
	uint8_t d1 = rb[i0];
	rb_r++;
	return (int16_t)( ((uint16_t)d1 << 8) | (uint16_t)d0 );

}

static uint8_t initsound(FATFS* fs, FIL * fil, fileData* fn, uint8_t song){
	rb_w = 0;
	rb_r = 0;
	need_refill = 0;
	eof_seen = 0;
	STOPSONG = 0;
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
	printstr("\n\rrb_r: ");
	uart_print_u32((uint32_t)rb_r);
	printstr("\n\rrb_w: ");
	uart_print_u32((uint32_t)rb_w);
	printstr("\n\rrb_count: ");
	uart_print_u32((uint32_t)rb_count());
	printstr("\n\rrb_space: ");
	uart_print_u32((uint32_t)rb_space());
	printstr("\n\rbr: ");
	uart_print_u32((uint32_t)br);

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
			err2 = f_read(fil, rb, HRB_SIZE - br1, &br2);
		}
		switch (err1 | err2){
		case (0):
			break;
		default:
			printstr("Playback Error. Error reading on loading buffer");
			return 1;
		}
		rb_w += br1 + br2;
		need_refill = 0;
		return 0;
}


static void audio_fill_dma_half_0(void){ // refill first half of i2s_tx[]
	uint16_t *dst = i2s_tx;
	 for (uint32_t f = 0; f < FR_PER_H; f++)
		  {
			switch (file_channels){
				case (2): //stereo
				  if (rb_count() >= 4u) {
					*dst++ = (uint16_t)rb_pop_i16();
					*dst++ = (uint16_t)rb_pop_i16();
				  }
				  else {
					  *dst++ = 0; *dst++ = 0;   // under-run => silence
					  }
					break;
				default: // mono
				  if (rb_count() >= 2u) {
					int16_t S = rb_pop_i16();
					*dst++ = (uint16_t)S;
					*dst++ = (uint16_t)S;
				  }
				  else {
					*dst++ = 0; *dst++ = 0;
				  }
				  break;
			}
		  // Tell main loop “top up the ring from SD soon”
			if (!eof_seen && rb_count() <= (RB_SIZE - 1u - HRB_SIZE)) {
			    need_refill = 1;
			}
		}
	}

static void audio_fill_dma_half_1(void){ // refill second half of i2s_tx[]
	uint16_t *dst = &i2s_tx[HW_PER_H];
	 for (uint32_t f = 0; f < FR_PER_H; f++)
	  {
	    switch (file_channels) {
	    	case (2): //stereo
			  if (rb_count() >= 4u) {
				*dst++ = (uint16_t)rb_pop_i16();;
				*dst++ = (uint16_t)rb_pop_i16();;
			  }
			  else {
				  *dst++ = 0; *dst++ = 0;   // under-run => silence
#ifdef DEBUGGIN
				  printstr("Error: Playback Under-run.");
				  exit(1);
				  underrun++;
#endif
				  }
				break;
	    	default: // mono
			  if (rb_count() >= 2u) {
				int16_t S = rb_pop_i16();
				*dst++ = (uint16_t)S;
				*dst++ = (uint16_t)S;
			  }
			  else {
				*dst++ = 0; *dst++ = 0;
#ifdef DEBUGGIN
				printstr("Error: Playback Under-run.");
				exit(1);
				underrun++;
#endif
			  }
			  break;
	    }
	  // Tell main loop “top up the ring from SD soon”
	  if (!eof_seen && rb_count() < (24u * 1024u)) {
	    need_refill = 1;
	  }
	}
}


void blockingPlaySound(fileData* fn, uint8_t song){ // Plays sound and holds control until STOPSONG flag raised
	FATFS fs;
	FIL fil;
	file_channels = fn->fmt[song].num_channel;
	if (initsound(&fs, &fil, fn, song)){
		return;
	}
	audio_fill_dma_half_0();
	audio_fill_dma_half_1();
	HAL_I2S_Transmit_DMA(&hi2s2, i2s_tx, HW_PER_H * 2u);
	while (!(STOPSONG)){
		if ((rb_space() > HRB_SIZE)) {

			if (loadbuff(&fil) != 0){
				f_close(&fil);
				f_mount(NULL, "0:", 0);
				HAL_I2S_DMAStop(&hi2s2);
				return;
			}
		}
	}
}






/* Called by HAL when the FIRST half of the circular DMA buffer finished transmitting */
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if (hi2s->Instance != hi2s2.Instance) return;  // ignore other I2S instances
  audio_fill_dma_half_0();
  return;
}

/* Called by HAL when the SECOND half finished transmitting */
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if (hi2s->Instance != hi2s2.Instance) return;
  audio_fill_dma_half_1();
  return;
}

/* Called by HAL if it detects an I2S/DMA error (underrun, DMA error, etc.) */
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
  if (hi2s->Instance != hi2s2.Instance) return;
  I2SERR = 1;
  STOPSONG = 1;
  return;
}

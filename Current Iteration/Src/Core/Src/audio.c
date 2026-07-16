/*
 * audio.c
 *
 *  Created on: Feb 8, 2026
 *      Author: Apath
 */

//#include "app_fatfs.h"
#include "audio.h"
#include "debugging.h"
#include <string.h>
#include "SD_test.h"

#define RB_SIZE (64u * 1024u)   // 64KB ring buffer, must be a power of two
#define RB_MASK (RB_SIZE - 1u)	// 0xFFFF, mask for ring buffer
#define HRB_SIZE (RB_SIZE/2)	// half of a ring buffer
#define FR_PER_H (1024u * 4u)	//
#define HW_PER_FR 2
#define HW_PER_H (FR_PER_H * HW_PER_FR)

#define NUM_INTERNAL_SONGS 40 	// used in audio_map_names/indexes , can be increased if more tracks are desired
#define NUM_USER_SONGS 40		// Max number of user songs

static uint8_t read_buffer[RB_SIZE];
static volatile uint32_t rb_w = 0, rb_r = 0;
static volatile uint8_t STOPSONG = 0;
volatile uint8_t I2SERR = 0;

extern I2S_HandleTypeDef hi2s2;
static uint16_t i2s_tx[HW_PER_H * 2u];
static volatile uint8_t need_refill = 0;
static volatile uint8_t eof_seen = 0;
static uint8_t file_channels = 0;
static volatile uint16_t overwrite = 0;
static volatile uint16_t underrun = 0;
uint8_t counted_user_songs = 0;
fileData fn = {0};
static uint8_t audio_mapping();


const char *audio_map_names[NUM_INTERNAL_SONGS] = {
		"AL1_ON", 					//0 - Voice tracks
		"AL1_OFF",
		"AL2_ON",
		"AL2_OFF",
		"SET_TIME",
		"SET_AL1",
		"SET_AL2",
		"SET_BRIGHTNESS",
		"SET_VOL",
		"SET_AL_TIME",
		"SET_AL_SONG",				// 10
		"SET_AL_DAYS",
		"SET_AL_ON",
		"SET_AL_OFF",
		"SET_RETURN_MAIN",
		"EDITING",
		"ENTERING_ALARM_MENU",
		"MAIN_MENU",
		"CONFIRMED",
		"",
		"",							// 20
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"AFFIRM", 					// 30 - Sound Tracks - must start at index 30 or Userinterface.c will contain incorrect enumerations for the soundTracks enumerated type, i.e. the song indexed for AFFIRM no longer map to the AFFIRM playSound call
		"INC",
		"DEC",
		"",
		"",
		"",
		"",
		"",
		"",
		"" 							// 39
};

uint8_t audio_map_index[NUM_INTERNAL_SONGS + NUM_USER_SONGS] = {
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 	// 0 -15
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,	// 16 - 31
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 	// 32 - 47
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,	// 48 - 63
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,	// 64 - 79
};

volatile audio_mode_t audio_mode = AUDIO_NORMAL;

// Reseting this pin turns off the MAX98357A audio amplifier, when not in use
void audio_shutdown(){
	HAL_GPIO_WritePin(AMPLIFIER_SD_PORT, AMPLIFIER_SD_PIN, GPIO_PIN_RESET);
	return;
}

// Setting this pin turns on the MAX9857A audio amplifier.
void audio_startup(){
	HAL_GPIO_WritePin(AMPLIFIER_SD_PORT, AMPLIFIER_SD_PIN, GPIO_PIN_SET);
	return;
}

// counts the number of bytes left in the read buffer from the index
static inline uint32_t rb_count(void) {
  return (rb_w - rb_r) ;
}

/*
static inline uint32_t rb_space(void) {
  return (RB_SIZE  - rb_count() );
}
*/

// return one 16 bit sample from the buffer, update
static inline int16_t rb_pop_i16(void){
	if (rb_count() < 2u) return 0;	// fail if not enough bytes left in buffer
	uint32_t i0 = rb_r & RB_MASK;	// mask rb_r to size of read buffer
	uint8_t d0 = read_buffer[i0];	// read byte one
	rb_r++;							// increment index
	i0 = rb_r & RB_MASK;			// mask again
	uint8_t d1 = read_buffer[i0];	// read byte two
	rb_r++;							// increment again
	return (int16_t)( ((uint16_t)d1 << 8) | (uint16_t)d0 );	// send data with same endian-ness
}

static uint8_t initsound(FATFS* fs, FIL * fil, fileData* fn, uint8_t song){
	// Initialize the read buffer and related parameters
	// Mount SD, open file handed in fil argument, go to start of data, read the initial data entry
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
	uint8_t test[16];

	f_read(fil, test, 16, &br);										// read first 16 bytes
#ifdef DEBUGGIN
	for (int i = 0; i < 16; i++) {
	    printf("%02X ", test[i]);
	}
	printf("\r\n");
#endif
	err = f_read(fil, read_buffer, RB_SIZE, &br);
	switch (err){
		case (0):
			break;
		default:
			printstr("Playback Error. Error reading on intitiate song");
			return 1;
	}
	rb_w = br;	// set read buffer index to number of bytes read

#ifdef DEBUGGIN
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
#endif

	// read_buffer now holds first read, rb_r and rb_r are indexed to the proper locations.
	return 0;
}


/*
static inline uint8_t loadbuff(FIL * fil){
		UINT br1 = 0;
		UINT br2 = 0;
		uint32_t write = rb_w & (RB_SIZE - 1);
		uint8_t err1 = 0;
		uint8_t err2 = 0;
		if (write < HRB_SIZE){
			err1 = f_read(fil, &read_buffer[write], HRB_SIZE, &br1);
		}
		else{
			err1 = f_read(fil, &read_buffer[write], RB_SIZE - write, &br1);
			err2 = f_read(fil, read_buffer, HRB_SIZE - br1, &br2);
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
*/

static void audio_fill_dma_half_0(void){ // refill first half of i2s_tx[]
	uint16_t *dst = i2s_tx; // dst points to first half of i2s_tx buffer
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
	uint16_t *dst = &i2s_tx[HW_PER_H]; // dst points to second half of i2s_tx buffer
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
#endif
				  underrun++;

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
#endif
				underrun++;

			  }
			  break;
	    }
	  // Tell main loop “top up the ring from SD soon”
	  if (!eof_seen && rb_count() < (24u * 1024u)) {
	    need_refill = 1;
	  }
	}
}


void playSound(uint8_t song){
	// Plays sound and holds control until STOPSONG flag raised
	fileData *fnp = &fn;
	// Make sure files are mapped to their index for internal audio files
	static uint8_t audio_mapping_incomplete = 1;
	if (audio_mapping_incomplete){
		audio_mapping_incomplete = audio_mapping();
		if (audio_mapping_incomplete){
			printstr("Error, audio mapping failure.\n\r");
		}
	}
	uint8_t song_index = audio_map_index[song];
	printstr("\n\rAttempting to play song: ");
	printstr(fn.name[song_index]);
	printstr(" -> Ind: ");
	uart_print_u32((uint32_t)song);
	printstr("  -> Hash: ");
	uart_print_u32((uint32_t)song_index);
	printstr("\n");
	if (song_index == 255){
		printstr("Error, requested song which does not exist.\n\r");
		return;
	}
	audio_startup(); // Enable amplifier
	FATFS fs;
	FIL fil;
	file_channels = fnp->fmt[song_index].num_channel;
	if (initsound(&fs, &fil, fnp, song_index)){ // if failure, fail
		printstr("Error: audio initiation failed.\n\r");
		audio_shutdown();
		return;
	}
	// init I2S buffers
	audio_fill_dma_half_0();
	audio_fill_dma_half_1();


	HAL_I2S_Transmit_DMA(&hi2s2, i2s_tx, HW_PER_H * 2u);
	/*
	while (!(STOPSONG)){

		if ((rb_space() > HRB_SIZE)) {

			if (loadbuff(&fil) != 0){
				f_close(&fil);
				f_mount(NULL, "0:", 0);
				HAL_I2S_DMAStop(&hi2s2);
				break;
			}
		}
	}
		*/

	// turn off amplifier at enable pin
	audio_shutdown();
	return;
}

void print_audio_mapping_debug(){
	printstr("\n\r");
	for (int i = 0; i < 50; i++){
		printstr("Fname: ");
		printstr(fn.name[i]);
		printstr("\r\t\t\tHash: ");
		uart_print_u32((uint32_t)audio_map_index[i]);
		printstr("\r\t\t\t\t\tIndex: ");
		uart_print_u32((uint32_t)i);
		printstr("\r\t\t\t\t\t\t\tValid: ");
		uart_print_u32((uint32_t)fn.fmt[i].valid);
		printstr("\n\r");
	}
}

void print_audio_mapping_debug2(){
	printstr("\n\r");
	for (int i = 0; i < NUM_INTERNAL_SONGS + NUM_USER_SONGS; i++){
		if (i < NUM_INTERNAL_SONGS){
		printstr("Literal: ");
		char buf[50];
		strcpy(buf, audio_map_names[i]);
		printstr(buf);}

		printstr("\r\t\t\tHash: ");
		uart_print_u32((uint32_t)audio_map_index[i]);
		printstr("\r\t\t\t\t\tIndex: ");
		uart_print_u32((uint32_t)i);
		printstr("\n\r");
	}
}

static uint8_t audio_mapping(){ // maps the file names stored in
	uint8_t k = 0;
	for (int i = 0; i < (NUM_WAVE_FILES + k); i++){
		if (fn.fmt[i].valid == 0){k++;continue;}
		for (int j = 0; j < NUM_INTERNAL_SONGS; j++){
			if (  !(strncmp(fn.name[i], audio_map_names[j], strlen(audio_map_names[j])))  && (strcmp(audio_map_names[j], "")) ) {
				audio_map_index[j] = i;
				break;
			}
			if (j == (NUM_INTERNAL_SONGS - 1)){
				// interprets a song not found as a user added song and adds it to the mapping starting at the user song offset of 40
				audio_map_index[NUM_INTERNAL_SONGS + counted_user_songs] = i;
				counted_user_songs++;
			}
		}
	}
	print_audio_mapping_debug();
	print_audio_mapping_debug2();
	if (NUM_WAVE_FILES == 0){return 1;}
	return 0;
}

static void fill_tone_half(uint32_t half)
{
    uint16_t *dst = (half == 0) ? i2s_tx : &i2s_tx[HW_PER_H];
    for (uint32_t i = 0; i < HW_PER_H; i += 2) {
        int16_t sample = ((i / 96u) & 1u) ? 2000 : -2000;
        dst[i]     = (uint16_t)sample;
        dst[i + 1] = (uint16_t)sample;
    }
}

static void fill_silence_half(uint32_t half)
{
    uint16_t *dst = (half == 0) ? i2s_tx : &i2s_tx[HW_PER_H];
    for (uint32_t i = 0; i < HW_PER_H; i++) {
        dst[i] = 0x0000;
    }
}


void test_i2s_tone(void) // Test a quiet tone generation if you can't get audio working through the audio player
{
    audio_mode = AUDIO_MODE_TONE;
    fill_tone_half(0);
    fill_tone_half(1);
    HAL_I2S_Transmit_DMA(&hi2s2, i2s_tx, HW_PER_H * 2u);
}

void test_i2s_silence(void)  // Test a (nearly) silent tone generation if you can't get audio working through the audio player
{
    audio_mode = AUDIO_MODE_SILENCE;
    fill_silence_half(0);
    fill_silence_half(1);
    HAL_I2S_Transmit_DMA(&hi2s2, i2s_tx, HW_PER_H * 2u);
}


/* Debugging callback for using test_i2s_tone and test i2s silence
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance != hi2s2.Instance) return;

    if (audio_mode == AUDIO_MODE_SILENCE) {
        fill_silence_half(0);
    } else if (audio_mode == AUDIO_MODE_TONE) {
        fill_tone_half(0);
    } else if (audio_mode == AUDIO_MODE_WAV) {
        audio_fill_dma_half_0();
    }
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance != hi2s2.Instance) return;

    if (audio_mode == AUDIO_MODE_SILENCE) {
        fill_silence_half(1);
    } else if (audio_mode == AUDIO_MODE_TONE) {
        fill_tone_half(1);
    } else if (audio_mode == AUDIO_MODE_WAV) {
        audio_fill_dma_half_1();
    }
}
*/
// Called by HAL when the FIRST half of the circular DMA buffer finished transmitting
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if (hi2s->Instance != hi2s2.Instance) return;  // ignore other I2S instances
  switch (audio_mode){
  case (AUDIO_NORMAL):
		  audio_fill_dma_half_0();
		  break;
  case (AUDIO_LOW_POWER):
		  break;
  default:
	  break;
  }
  return;
}

//Called by HAL when the SECOND half finished transmitting
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if (hi2s->Instance != hi2s2.Instance) return;
  switch (audio_mode){
  case (AUDIO_NORMAL):
		  audio_fill_dma_half_1();
		  break;
  case (AUDIO_LOW_POWER):
		  break;
  default:
	  break;
  }
  return;
}

// Called by HAL if it detects an I2S/DMA error (underrun, DMA error, etc.) */
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
  if (hi2s->Instance != hi2s2.Instance) return;
  I2SERR = 1;
  STOPSONG = 1;
  return;
}

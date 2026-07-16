/*
 * audio.h
 *
 *  Created on: Feb 8, 2026
 *      Author: Apath
 */

#ifndef INC_AUDIO_H_
#define INC_AUDIO_H_
#include "SD_test.h"
#include <stdint.h>
#define AMPLIFIER_SD_PORT  GPIOB
#define AMPLIFIER_SD_PIN	GPIO_PIN_2
extern SPI_HandleTypeDef hspi1;
void playSound(uint8_t song);
typedef enum {
    AUDIO_NORMAL = 0,
	AUDIO_LOW_POWER,
    AUDIO_MODE_TONE,
	AUDIO_MODE_SILENCE,
    AUDIO_MODE_WAV
} audio_mode_t;
extern volatile audio_mode_t audio_mode;
extern uint8_t counted_user_songs;
void audio_shutdown();
void audio_startup();
extern fileData fn;

#endif /* INC_AUDIO_H_ */

/*
 * SD_test.h
 *
 *  Created on: Feb 7, 2026
 *      Author: Apath
 */

#ifndef INC_SD_TEST_H_
#define INC_SD_TEST_H_
#include "app_fatfs.h"
#define MAX_SONGS 50

typedef struct fmt {
	uint32_t file_size;
	uint16_t audio_type;
	uint16_t num_channel;
	uint32_t sample_rate;
	uint16_t bitpersample;
	uint32_t data_size;
	uint32_t datastart;
	uint8_t valid;

} fmt;

typedef struct fileData{
	char name[MAX_SONGS][20];
	struct fmt fmt[MAX_SONGS];
}fileData;


extern uint8_t NUMSONGS;
void UART_Print(char* str);
void SD_Card_Test(void);
void SD_init(FATFS* fs, fileData *fn);


void parsewavheader(FIL *fil, struct fmt * fmt);

#endif /* INC_SD_TEST_H_ */

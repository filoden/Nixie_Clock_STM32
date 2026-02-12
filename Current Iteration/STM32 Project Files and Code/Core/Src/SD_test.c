/*
 * SD_test.c
 *
 *  Created on: Feb 7, 2026
 *      Author: Apath
 */
#define _GNU_SOURCE // for memmem() function

#include "SD_test.h"
#include "spi.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "debugging.h"
#include "diskio.h"
#include <stdarg.h>


//SPI_HandleTypeDef hspi2;
extern UART_HandleTypeDef huart2;
char TxBuffer[250];
uint8_t NUMSONGS = 0;
FRESULT FR = 0;

void UART_Print(char* str)
{
    HAL_UART_Transmit(&huart2, (uint8_t *) str, strlen(str), 100);
}

void SD_Card_Test(void)
{
  FATFS FatFs;
  FIL Fil;
  FRESULT FR_Status;
  UINT RWC, WWC; // Read/Write Word Counter
  char RW_Buffer[200];
  do
  {
    //------------------[ Mount The SD Card ]--------------------
    FR_Status = f_mount(&FatFs, "", 1);
    if (FR_Status != FR_OK)
    {
      sprintf(TxBuffer, "Error! While Mounting SD Card, Error Code: (%i)\r\n", FR_Status);
      UART_Print(TxBuffer);
      break;

    }
    sprintf(TxBuffer, "SD Card Mounted Successfully! \r\n\n");
    UART_Print(TxBuffer);
    //------------------[ Get & Print The SD Card Size & Free Space ]--------------------
    /*
    f_getfree("", &FreeClusters, &FS_Ptr);
    TotalSize = (uint32_t)((FS_Ptr->n_fatent - 2) * FS_Ptr->csize * 0.5);
    FreeSpace = (uint32_t)(FreeClusters * FS_Ptr->csize * 0.5);
    sprintf(TxBuffer, "Total SD Card Size: %lu Bytes\r\n", TotalSize);
    UART_Print(TxBuffer);
    sprintf(TxBuffer, "Free SD Card Space: %lu Bytes\r\n\n", FreeSpace);
    UART_Print(TxBuffer);
    */
    //------------------[ Open A Text File For Write & Write Data ]--------------------
    //Open the file
    FR_Status = f_open(&Fil, "TextFW.txt", FA_WRITE | FA_READ | FA_CREATE_ALWAYS);
    if(FR_Status != FR_OK)
    {
      sprintf(TxBuffer, "Error! While Creating/Opening A New Text File, Error Code: (%i)\r\n", FR_Status);
      UART_Print(TxBuffer);
      break;
    }
    sprintf(TxBuffer, "Text File Created & Opened! Writing Data To The Text File..\r\n\n");
    UART_Print(TxBuffer);
    // (1) Write Data To The Text File [ Using f_puts() Function ]
    f_puts("Hello! From STM32 To SD Card Over SPI, Using f_puts()\n", &Fil);
    // (2) Write Data To The Text File [ Using f_write() Function ]
    strcpy(RW_Buffer, "Hello! From STM32 To SD Card Over SPI, Using f_write()\r\n");
    f_write(&Fil, RW_Buffer, strlen(RW_Buffer), &WWC);
    // Close The File
    f_close(&Fil);
    //------------------[ Open A Text File For Read & Read Its Data ]--------------------
    // Open The File
    FR_Status = f_open(&Fil, "TextFW.txt", FA_READ);
    if(FR_Status != FR_OK)
    {
      sprintf(TxBuffer, "Error! While Opening (TextFileWrite.txt) File For Read.. \r\n");
      UART_Print(TxBuffer);
      break;
    }
    // (1) Read The Text File's Data [ Using f_gets() Function ]
    f_gets(RW_Buffer, sizeof(RW_Buffer), &Fil);
    sprintf(TxBuffer, "Data Read From (TextFileWrite.txt) Using f_gets():%s", RW_Buffer);
    UART_Print(TxBuffer);
    // (2) Read The Text File's Data [ Using f_read() Function ]
    f_read(&Fil, RW_Buffer, f_size(&Fil), &RWC);
    sprintf(TxBuffer, "Data Read From (TextFileWrite.txt) Using f_read():%s", RW_Buffer);
    UART_Print(TxBuffer);
    // Close The File
    f_close(&Fil);
    sprintf(TxBuffer, "File Closed! \r\n\n");
    UART_Print(TxBuffer);
    //------------------[ Open An Existing Text File, Update Its Content, Read It Back ]--------------------
    // (1) Open The Existing File For Write (Update)
    FR_Status = f_open(&Fil, "TextFW.txt", FA_OPEN_EXISTING | FA_WRITE);
    FR_Status = f_lseek(&Fil, f_size(&Fil)); // Move The File Pointer To The EOF (End-Of-File)
    if(FR_Status != FR_OK)
    {
      sprintf(TxBuffer, "Error! While Opening (TextFileWrite.txt) File For Update.. \r\n");
      UART_Print(TxBuffer);
      break;
    }
    // (2) Write New Line of Text Data To The File
    FR_Status = f_puts("This New Line Was Added During Update!\r\n", &Fil);
    f_close(&Fil);
    memset(RW_Buffer,'\0',sizeof(RW_Buffer)); // Clear The Buffer
    // (3) Read The Contents of The Text File After The Update
    FR_Status = f_open(&Fil, "TextFW.txt", FA_READ); // Open The File For Read
    f_read(&Fil, RW_Buffer, f_size(&Fil), &RWC);
    sprintf(TxBuffer, "Data Read From (TextFileWrite.txt) After Update:%s", RW_Buffer);
    UART_Print(TxBuffer);
    f_close(&Fil);
    //------------------[ Delete The Text File ]--------------------
    // Delete The File
    /*
    FR_Status = f_unlink(TextFileWrite.txt);
    if (FR_Status != FR_OK){
        sprintf(TxBuffer, "Error! While Deleting The (TextFileWrite.txt) File.. \r\n");
        UART_Print(TxBuffer);
    }
    */
  } while(0);
  //------------------[ Test Complete! Unmount The SD Card ]--------------------
  FR_Status = f_mount(NULL, "", 0);
  if (FR_Status != FR_OK)
  {
      sprintf(TxBuffer, "Error! While Un-mounting SD Card, Error Code: (%i)\r\n", FR_Status);
      UART_Print(TxBuffer);
  } else{
      sprintf(TxBuffer, "SD Card Un-mounted Successfully! \r\n");
      UART_Print(TxBuffer);
  }
}

static void getfilenames(fileData * fn){
	FIL fil;
	uint8_t fr = f_open(&fil, "0:/songname.txt", FA_OPEN_EXISTING | FA_READ);
	if (FR){print_fopen_error(FR);}
	  int i = 0;
	  while (!(f_eof(&fil)) && (i< MAX_SONGS)){
		  if (f_gets(fn->name[i], sizeof(fn->name[i]), &fil) == NULL) {
			  break; // read error or EOF
		  	  }
		  fn->name[i][strcspn(fn->name[i], "\r\n")] = '\0';
		  fn->name[i][strcspn(fn->name[i], "\n")] = '\0';
		  i++;
	  }
	  f_close(&fil);
	  if (FR){print_fclose_error(FR);}
	  NUMSONGS = i;
	  return;

}

static void printtable(uint8_t i, fileData * fn){
	printstr("\n\r\n");
	printstr("--- Audio File Information: ---");
	printstr("\n\r\nFIle name: ");
	printstr(fn->name[i]);
	printstr("\n\rFile size: ");
	uart_print_u32(fn->fmt[i].file_size);
	printstr("\n\rValid?:");
	uart_print_u32(fn->fmt[i].valid);
	printstr("\n\rOffset: ");
	uart_print_u32(fn->fmt[i].datastart);
	printstr("\n\rAudio Type: ");
	uart_print_u32((uint32_t)fn->fmt[i].audio_type);
	printstr("\n\rBits Per Sample: ");
	uart_print_u32((uint32_t)fn->fmt[i].bitpersample);
	return;
}


void SD_init(FATFS* fs, fileData *fn){
	FIL Fil;
	FR = f_mount(fs, "0:", 1);
	if (FR){print_fmount_error(FR);}

	getfilenames(fn);
	for (int i = 0;i<NUMSONGS;i++){
		FR = f_open(&Fil, fn->name[i], FA_OPEN_EXISTING | FA_READ);
		if (FR){print_fopen_error(FR);}
		switch (FR){
		case (0):
			parsewavheader(&Fil, &fn->fmt[i]);
			f_close(&Fil);
			if (FR){print_fclose_error(FR);}
			break;
		default:
			printstr("Unexpected Error opening song file.");
			break;
		}
	}
	for (int i = 0; i<MAX_SONGS; i++){
		if (fn->fmt[i].valid != 1){
			for (int j = i; j<( MAX_SONGS - 1);j++){
				fn->fmt[j] = fn->fmt[j+1];
				strcpy(fn->name[j],fn->name[j+1]);
			}
		}
	}
	int k = 0;
	while ( ((fn->fmt[k].valid) == 1) && k < MAX_SONGS){
		printtable(k, fn);
		k++;
	}
	return;
}

static uint32_t decodele4(unsigned char * start){
	return (
			((uint32_t)start[3] << 24) |
			((uint32_t)start[2] << 16) |
			((uint32_t)start[1] << 8) |
			(uint32_t)start[0]
							);
}

static uint16_t decodele2(unsigned char * start){
	return ( (uint16_t)start[1] << 8 | (uint16_t)start[0]);

}

void parsewavheader(FIL *fil, struct fmt * fmt1){
	//f_mount(&fs, "0:", 1);
	char buff[512] = "";
	//f_open(&fil, "0:/fjm.wav", FA_OPEN_EXISTING | FA_READ);
	UINT br;
	f_read(fil, buff, 512, &br);
	if (FR){print_fread_error(FR);}
	char * fmtbase = memmem(buff,512, "fmt ",4);
	char * database = memmem(buff,512, "data",4);
	uint8_t chk =
			(memcmp("RIFF", buff, 4) 	|
			memcmp("WAVE", &buff[8], 4));

	if (chk){
		printstr("Wav file error. File lacks proper header");
		fmt1->valid = 0;
		return;
	}
	if ((fmtbase == NULL) || (database == NULL)){
		printstr("Wav file error. Could not find either 'fmt ' or 'data' headers");
		fmt1->valid = 0;
		return;
	}
	fmt1->file_size =  decodele4((unsigned char*)&buff[4]);
	fmt1->audio_type = decodele2((unsigned char*)fmtbase+8);
	fmt1->num_channel = decodele2((unsigned char*)fmtbase+10);
	fmt1->sample_rate = decodele4((unsigned char*)fmtbase+12);
	fmt1->bitpersample = decodele2((unsigned char*)fmtbase+22);
	fmt1->data_size = decodele4((unsigned char*)database+4);
	fmt1->datastart = ((database+8)-buff); // move cursor to start of data, offset tp 512 to improve read latency
	uint8_t align = fmt1->datastart % 4;
	if (align){
		fmt1->datastart += align;
	}
	if (fmt1->audio_type != 1){
		fmt1->valid = 0;
		printstr("Wav file error. audio type not PCM");
		return;
	}
	if (fmt1->bitpersample != 16){
			fmt1->valid = 0;
			printstr("wav error: bps not 16");
			return;
		}
	switch(fmt1->num_channel){
	case (1):
			break;
	case (2):
			break;
	default:
		printstr("Wav file error. Number of channels not supported.");
	}
	switch (fmt1->sample_rate){
	case (8000):
			break;
	case (16000):
			break;
	case (32000):
			break;
	case (44100):
			break;
	case (48000):
			break;
	case (88200):
			break;
	case (96000):
			break;
	default :
		printstr("Wav file error. sample rate not supported");
		fmt1->valid = 0;
		return;
	}
	fmt1->valid = 1;
	return;
}



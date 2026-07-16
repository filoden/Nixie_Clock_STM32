/*
 * File: FATFS_SD.c
 * Driver Name: FATFS_SD SPI + DMA read payload
 * Layer: MIDWARE
 *
 * Notes:
 * - SD commands, response polling, token waits, and CRC discard remain blocking.
 * - The SD data payload receive is DMA-backed via HAL_SPI_TransmitReceive_DMA().
 * - Requires SPI RX and TX DMA to be enabled for HSPI_SDCARD in CubeMX.
 * - Requires DMA IRQs to call HAL_DMA_IRQHandler() from the generated IRQ handlers.
 */

#include "main.h"
#include "diskio.h"
#include "FATFS_SD.h"
#include "spi.h"
#include <string.h>

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef BYTE sd_bool_t;

/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

/*
 * Kept for compatibility with older FatFs-style timer code.
 * This driver mainly uses HAL_GetTick() timeouts, but SD_TimerProc()
 * is still provided at the bottom.
 */
volatile uint16_t Timer1, Timer2;

/* Card type flags: CT_MMC, CT_SD1, CT_SD2, CT_BLOCK are expected from FATFS_SD.h */
static uint8_t CardType;
static uint8_t PowerFlag = 0;

/*
 * SPI receive requires transmitting dummy bytes.
 * This buffer must remain valid for the full DMA transfer, so it is static.
 */
static uint8_t spi_sd_dummy_tx[512];
static uint8_t spi_sd_dummy_ready = 0;

/* -------------------------------------------------------------------------- */
/* GPIO chip-select helpers                                                    */
/* -------------------------------------------------------------------------- */

static void SELECT(void)
{
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET);
}

static void DESELECT(void)
{
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
}

/* -------------------------------------------------------------------------- */
/* SPI helpers                                                                 */
/* -------------------------------------------------------------------------- */

static void SPI_PrepareDummyTx(void)
{
    if (!spi_sd_dummy_ready) {
        memset(spi_sd_dummy_tx, 0xFF, sizeof(spi_sd_dummy_tx));
        spi_sd_dummy_ready = 1;
    }
}

static void SPI_TxByte(uint8_t data)
{
    (void)HAL_SPI_Transmit(HSPI_SDCARD, &data, 1, SPI_TIMEOUT);
}

static void SPI_TxBuffer(uint8_t *buffer, uint16_t len)
{
    (void)HAL_SPI_Transmit(HSPI_SDCARD, buffer, len, SPI_TIMEOUT);
}

/*
 * Single-byte clock.
 * Used for command responses, token waits, ready waits, and CRC discard.
 */
static uint8_t SPI_RxByte(void)
{
    uint8_t tx = 0xFF;
    uint8_t rx = 0xFF;

    (void)HAL_SPI_TransmitReceive(HSPI_SDCARD, &tx, &rx, 1, SPI_TIMEOUT);

    return rx;
}

/*
 * DMA transmit/receive helper.
 *
 * This waits for completion by polling the HAL SPI state, so it does not require
 * you to add a separate HAL_SPI_TxRxCpltCallback() just for this driver.
 *
 * It DOES require the generated DMA IRQ handlers to call HAL_DMA_IRQHandler().
 */
static sd_bool_t SPI_TxRxBufferDMA(uint8_t *tx, uint8_t *rx, uint16_t len, uint32_t timeout_ms)
{
    uint32_t t0;

    if (len == 0u) {
        return TRUE;
    }

    /* Wait until the SPI peripheral is idle before starting DMA. */
    t0 = HAL_GetTick();
    while (HAL_SPI_GetState(HSPI_SDCARD) != HAL_SPI_STATE_READY) {
        if ((HAL_GetTick() - t0) > timeout_ms) {
            return FALSE;
        }
    }

    if (HAL_SPI_TransmitReceive_DMA(HSPI_SDCARD, tx, rx, len) != HAL_OK) {
        return FALSE;
    }

    /* Wait for DMA completion. */
    t0 = HAL_GetTick();
    while (HAL_SPI_GetState(HSPI_SDCARD) != HAL_SPI_STATE_READY) {
        if ((HAL_GetTick() - t0) > timeout_ms) {
            (void)HAL_SPI_Abort(HSPI_SDCARD);
            return FALSE;
        }
    }

    /* Wait for SPI not busy after the DMA engine reports completion. */
    t0 = HAL_GetTick();
    while (__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_BSY)) {
        if ((HAL_GetTick() - t0) > timeout_ms) {
            (void)HAL_SPI_Abort(HSPI_SDCARD);
            return FALSE;
        }
    }

    return (HAL_SPI_GetError(HSPI_SDCARD) == HAL_SPI_ERROR_NONE) ? TRUE : FALSE;
}

/* -------------------------------------------------------------------------- */
/* SD protocol helpers                                                         */
/* -------------------------------------------------------------------------- */

static uint8_t SD_ReadyWait(void)
{
    uint8_t res;
    uint32_t t0 = HAL_GetTick();

    do {
        res = SPI_RxByte();

        if (res == 0xFFu) {
            return res;
        }
    } while ((HAL_GetTick() - t0) < 500u);

    return res;
}

static sd_bool_t SD_WaitReady(uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();

    do {
        if (SPI_RxByte() == 0xFFu) {
            return TRUE;
        }
    } while ((HAL_GetTick() - t0) < timeout_ms);

    return FALSE;
}

static void SD_PowerOn(void)
{
    uint8_t args[6];

    /* Send 80+ clocks with CS high to enter SPI mode. */
    DESELECT();

    for (uint8_t i = 0; i < 10u; i++) {
        SPI_TxByte(0xFF);
    }

    /* Send CMD0 once here to wake/idle the card. SD_disk_initialize() repeats it. */
    SELECT();

    args[0] = CMD0;
    args[1] = 0;
    args[2] = 0;
    args[3] = 0;
    args[4] = 0;
    args[5] = 0x95;

    SPI_TxBuffer(args, sizeof(args));

    uint32_t t0 = HAL_GetTick();
    while (SPI_RxByte() != 0x01u) {
        if ((HAL_GetTick() - t0) > 100u) {
            break;
        }
    }

    DESELECT();
    SPI_TxByte(0xFF);

    PowerFlag = 1;
}

static void SD_PowerOff(void)
{
    PowerFlag = 0;
}

static uint8_t SD_CheckPower(void)
{
    return PowerFlag;
}

/*
 * Receive an SD data block.
 *
 * The data token wait is blocking because it is a short state-machine phase.
 * The actual payload receive is DMA-backed.
 */
static sd_bool_t SD_RxDataBlock(BYTE *buff, UINT len)
{
    uint8_t token;
    uint32_t t0 = HAL_GetTick();
#ifdef DEBUGGIN
    printf("SD_RxDataBlock: waiting token, len=%u\r\n", len);
#endif
    do {
        token = SPI_RxByte();
#ifdef DEBUGGIN
        if (token != 0xFF) {
            printf("SD_RxDataBlock: token candidate=0x%02X\r\n", token);
        }
#endif
        if ((HAL_GetTick() - t0) > 200u) {
#ifdef DEBUGGIN
            printf("SD_RxDataBlock: token timeout, last=0x%02X\r\n", token);
#endif
            return FALSE;
        }
    } while (token == 0xFFu);

    if (token != 0xFEu) {
#ifdef DEBUGGIN
        printf("SD_RxDataBlock: bad token=0x%02X\r\n", token);
#endif
        return FALSE;
    }
#ifdef DEBUGGIN
    printf("SD_RxDataBlock: got 0xFE, starting DMA\r\n");
#endif
    SPI_PrepareDummyTx();

    if (!SPI_TxRxBufferDMA(spi_sd_dummy_tx, (uint8_t *)buff, (uint16_t)len, 50u)) {
#ifdef DEBUGGIN
        printf("SD_RxDataBlock: DMA receive failed\r\n");
#endif
        return FALSE;
    }
#ifdef DEBUGGIN
    printf("SD_RxDataBlock: DMA OK, sig=%02X %02X\r\n", buff[510], buff[511]);
#endif
    SPI_RxByte();
    SPI_RxByte();

    return TRUE;
}

/* -------------------------------------------------------------------------- */
/* SD write block helper                                                       */
/* -------------------------------------------------------------------------- */

#if _USE_WRITE == 1
static sd_bool_t SD_TxDataBlock(const uint8_t *buff, BYTE token)
{
    uint8_t resp = 0xFF;
    uint8_t i = 0;

    if (SD_ReadyWait() != 0xFFu) {
        return FALSE;
    }

    SPI_TxByte(token);

    if (token != 0xFDu) {
        SPI_TxBuffer((uint8_t *)buff, 512);

        /* Dummy CRC */
        SPI_RxByte();
        SPI_RxByte();

        while (i <= 64u) {
            resp = SPI_RxByte();

            if ((resp & 0x1Fu) == 0x05u) {
                break;
            }

            i++;
        }

        while (SPI_RxByte() == 0u) {
            ;
        }
    }

    return ((resp & 0x1Fu) == 0x05u) ? TRUE : FALSE;
}
#endif

/* -------------------------------------------------------------------------- */
/* Command helper                                                              */
/* -------------------------------------------------------------------------- */

/*
 * The CMDx macros are expected to already include the SPI command prefix.
 * Example:
 *   CMD0  = 0x40 + 0
 *   CMD17 = 0x40 + 17
 */
BYTE SD_SendCmd(BYTE cmd, uint32_t arg)
{
    uint8_t crc;
    uint8_t res;
    uint8_t n = 10;
    uint8_t ready;

    ready = SD_ReadyWait();

    if (ready != 0xFFu) {
#ifdef DEBUGGIN
        printf("SD_SendCmd: not ready before cmd=0x%02X, ready=0x%02X\r\n", cmd, ready);
#endif
        return 0xFFu;
    }
#ifdef DEBUGGIN
    printf("SD_SendCmd: cmd=0x%02X arg=%lu\r\n",
           cmd, (unsigned long)arg);
#endif
    SPI_TxByte(cmd);
    SPI_TxByte((uint8_t)(arg >> 24));
    SPI_TxByte((uint8_t)(arg >> 16));
    SPI_TxByte((uint8_t)(arg >> 8));
    SPI_TxByte((uint8_t)arg);

    if (cmd == CMD0) {
        crc = 0x95u;
    } else if (cmd == CMD8) {
        crc = 0x87u;
    } else {
        crc = 0x01u;
    }

    SPI_TxByte(crc);

    if (cmd == CMD12) {
        (void)SPI_RxByte();
    }

    do {
        res = SPI_RxByte();
    } while ((res & 0x80u) && --n);
#ifdef DEBUGGIN
    printf("SD_SendCmd: cmd=0x%02X response=0x%02X n_left=%u\r\n",
           cmd, res, n);
#endif

    return res;
}

/* -------------------------------------------------------------------------- */
/* FatFs diskio functions                                                      */
/* -------------------------------------------------------------------------- */

DSTATUS SD_disk_initialize(BYTE drv)
{
    uint8_t n;
    uint8_t type;
    uint8_t ocr[4];

    if (drv) {
        return STA_NOINIT;
    }

    if (Stat & STA_NODISK) {
        return Stat;
    }

    SD_PowerOn();

    SELECT();

    type = 0;

    if (SD_SendCmd(CMD0, 0) == 1u) {
        uint32_t t0 = HAL_GetTick();

        if (SD_SendCmd(CMD8, 0x1AAu) == 1u) {
            for (n = 0; n < 4u; n++) {
                ocr[n] = SPI_RxByte();
            }

            if (ocr[2] == 0x01u && ocr[3] == 0xAAu) {
                do {
                    if (SD_SendCmd(CMD55, 0) <= 1u &&
                        SD_SendCmd(CMD41, 1UL << 30) == 0u) {
                        break;
                    }
                } while ((HAL_GetTick() - t0) < 1000u);

                if ((HAL_GetTick() - t0) < 1000u && SD_SendCmd(CMD58, 0) == 0u) {
                    for (n = 0; n < 4u; n++) {
                        ocr[n] = SPI_RxByte();
                    }

                    type = (ocr[0] & 0x40u) ? (CT_SD2 | CT_BLOCK) : CT_SD2;
                }
            }
        } else {
            if (SD_SendCmd(CMD55, 0) <= 1u && SD_SendCmd(CMD41, 0) <= 1u) {
                type = CT_SD1;
            } else {
                type = CT_MMC;
            }

            t0 = HAL_GetTick();

            do {
                if (type == CT_SD1) {
                    if (SD_SendCmd(CMD55, 0) <= 1u && SD_SendCmd(CMD41, 0) == 0u) {
                        break;
                    }
                } else {
                    if (SD_SendCmd(CMD1, 0) == 0u) {
                        break;
                    }
                }
            } while ((HAL_GetTick() - t0) < 1000u);

            if ((HAL_GetTick() - t0) >= 1000u || SD_SendCmd(CMD16, 512) != 0u) {
                type = 0;
            }
        }
    }

    CardType = type;

    DESELECT();
    SPI_RxByte();

    if (type) {
        Stat &= (DSTATUS)~STA_NOINIT;
    } else {
        SD_PowerOff();
        Stat |= STA_NOINIT;
    }

    return Stat;
}

DSTATUS SD_disk_status(BYTE drv)
{
    if (drv) {
        return STA_NOINIT;
    }

    return Stat;
}

DRESULT SD_disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    UINT remaining;
    DWORD addr = sector;
#ifdef DEBUGGIN
    printf("SD_disk_read: LBA=%lu count=%u CardType=0x%02X Stat=0x%02X\r\n",
           (unsigned long)sector, count, CardType, Stat);
#endif

    if (pdrv || !count) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    if (!(CardType & CT_BLOCK)) {
        addr *= 512u;
    }
#ifdef DEBUGGIN
    printf("SD_disk_read: CMD address=%lu\r\n", (unsigned long)addr);
#endif
    SELECT();

    if (!SD_WaitReady(100u)) {
#ifdef DEBUGGIN
        printf("SD_disk_read: SD_WaitReady failed\r\n");
#endif
        DESELECT();
        SPI_RxByte();
        return RES_NOTRDY;
    }

    remaining = count;

    if (count == 1u) {
        BYTE r1 = SD_SendCmd(CMD17, addr);
#ifdef DEBUGGIN
        printf("CMD17 R1=0x%02X\r\n", r1);
#endif
        if ((r1 == 0u) && SD_RxDataBlock(buff, 512u)) {
            remaining = 0;
#ifdef DEBUGGIN
            printf("SD_RxDataBlock OK sig=%02X %02X\r\n", buff[510], buff[511]);
#endif
        }
#ifdef DEBUGGIN
        else {
            printf("SD_RxDataBlock failed\r\n");
        }
#endif
    }

    else {
#ifdef DEBUGGIN
        printf("CMD18 multi-read requested, count=%u\r\n", count);

#endif
        if (SD_SendCmd(CMD18, addr) == 0u) {
            do {
                if (!SD_RxDataBlock(buff, 512u)) {
#ifdef DEBUGGIN
                    printf("CMD18 block receive failed\r\n");
#endif
                    break;
                }

                buff += 512u;
            } while (--remaining);

            (void)SD_SendCmd(CMD12, 0);
            (void)SD_WaitReady(250u);
        }
    }

    DESELECT();
    SPI_RxByte();

    return remaining ? RES_ERROR : RES_OK;
}

#if _USE_WRITE == 1
DRESULT SD_disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    if (pdrv || !count) {
        return RES_PARERR;
    }

    if (Stat & STA_NOINIT) {
        return RES_NOTRDY;
    }

    if (Stat & STA_PROTECT) {
        return RES_WRPRT;
    }

    if (!(CardType & CT_BLOCK)) {
        sector *= 512u;
    }

    SELECT();

    if (count == 1u) {
        if ((SD_SendCmd(CMD24, sector) == 0u) && SD_TxDataBlock(buff, 0xFEu)) {
            count = 0;
        }
    } else {
        if (CardType & CT_SD1) {
            (void)SD_SendCmd(CMD55, 0);
            (void)SD_SendCmd(CMD23, count);
        }

        if (SD_SendCmd(CMD25, sector) == 0u) {
            do {
                if (!SD_TxDataBlock(buff, 0xFCu)) {
                    break;
                }

                buff += 512u;
            } while (--count);

            if (!SD_TxDataBlock(0, 0xFDu)) {
                count = 1;
            }
        }
    }

    DESELECT();
    SPI_RxByte();

    return count ? RES_ERROR : RES_OK;
}
#endif

DRESULT SD_disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
    DRESULT res;
    uint8_t n;
    uint8_t csd[16];
    uint8_t *ptr = (uint8_t *)buff;
    DWORD csize;

    if (drv) {
        return RES_PARERR;
    }

    if (ctrl == CTRL_POWER) {
        switch (*ptr) {
        case 0:
            SD_PowerOff();
            return RES_OK;

        case 1:
            SD_PowerOn();
            return RES_OK;

        case 2:
            *(ptr + 1) = SD_CheckPower();
            return RES_OK;

        default:
            return RES_PARERR;
        }
    }

    if (Stat & STA_NOINIT) {
        return RES_NOTRDY;
    }

    res = RES_ERROR;

    SELECT();

    switch (ctrl) {
    case CTRL_SYNC:
        if (SD_ReadyWait() == 0xFFu) {
            res = RES_OK;
        }
        break;

    case GET_SECTOR_COUNT:
        if ((SD_SendCmd(CMD9, 0) == 0u) && SD_RxDataBlock(csd, 16u)) {
            if ((csd[0] >> 6) == 1u) {
                /*
                 * CSD version 2.0:
                 * sector count = (C_SIZE + 1) * 1024
                 */
                csize = ((DWORD)(csd[7] & 0x3Fu) << 16) |
                        ((DWORD)csd[8] << 8) |
                        (DWORD)csd[9];

                *(DWORD *)buff = (csize + 1u) << 10;
            } else {
                /*
                 * CSD version 1.0:
                 * sector count from C_SIZE, C_SIZE_MULT, READ_BL_LEN
                 */
                n = (csd[5] & 15u) +
                    ((csd[10] & 128u) >> 7) +
                    ((csd[9] & 3u) << 1) + 2u;

                csize = (DWORD)(csd[8] >> 6) +
                        ((DWORD)csd[7] << 2) +
                        ((DWORD)(csd[6] & 3u) << 10) + 1u;

                *(DWORD *)buff = csize << (n - 9u);
            }

            res = RES_OK;
        }
        break;

    case GET_SECTOR_SIZE:
        *(WORD *)buff = 512u;
        res = RES_OK;
        break;

#ifdef GET_BLOCK_SIZE
    case GET_BLOCK_SIZE:
        /*
         * Erase block size in units of sectors. A conservative value of 1 is OK
         * for basic FatFs operation and f_mkfs compatibility.
         */
        *(DWORD *)buff = 1u;
        res = RES_OK;
        break;
#endif

    case MMC_GET_CSD:
        if (SD_SendCmd(CMD9, 0) == 0u && SD_RxDataBlock(ptr, 16u)) {
            res = RES_OK;
        }
        break;

    case MMC_GET_CID:
        if (SD_SendCmd(CMD10, 0) == 0u && SD_RxDataBlock(ptr, 16u)) {
            res = RES_OK;
        }
        break;

    case MMC_GET_OCR:
        if (SD_SendCmd(CMD58, 0) == 0u) {
            for (n = 0; n < 4u; n++) {
                *ptr++ = SPI_RxByte();
            }

            res = RES_OK;
        }
        break;

    default:
        res = RES_PARERR;
        break;
    }

    DESELECT();
    SPI_RxByte();

    return res;
}

/*
 * Optional 1ms timer hook for compatibility with FatFs-style disk timer code.
 * Call from SysTick or a 1ms timer ISR if other parts of your project use Timer1/2.
 */
void SD_TimerProc(void)
{
    if (Timer1) {
        Timer1--;
    }

    if (Timer2) {
        Timer2--;
    }
}

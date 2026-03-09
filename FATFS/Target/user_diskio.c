/**
 * @file user_diskio.c
 * @brief FatFS diskio через HAL_SD (polling mode).
 *
 * ИСПРАВЛЕНИЯ v2.4 SINGLE DRIVER:
 * 1. Только один драйвер USER_Driver
 * 2. Инициализация SDIO в USER_initialize
 * 3. Убран конфликт с SD_Driver
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "ff_gen_drv.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_sd.h"
#include "stm32f4xx_hal_uart.h"

extern SD_HandleTypeDef hsd;
extern UART_HandleTypeDef huart1;

#define SD_TIMEOUT_MS 10000U
#define SD_READY_WAIT_MS 2000U

static volatile DSTATUS Stat = STA_NOINIT;

/* C-логгер */
static void diskio_log(const char *fmt, ...)
{
    char buf[160];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, (uint16_t)len, 200);
    }
}

/* Прототипы */
DSTATUS USER_initialize(BYTE pdrv);
DSTATUS USER_status(BYTE pdrv);
DRESULT USER_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
DRESULT USER_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif
#if _USE_IOCTL == 1
DRESULT USER_ioctl(BYTE pdrv, BYTE cmd, void *buff);
#endif

Diskio_drvTypeDef USER_Driver = {
    USER_initialize,
    USER_status,
    USER_read,
#if _USE_WRITE
    USER_write,
#endif
#if _USE_IOCTL == 1
    USER_ioctl,
#endif
};

/* Ждём TRANSFER state */
static uint8_t sd_wait_ready(uint32_t timeout_ms)
{
    uint32_t tick = HAL_GetTick();
    HAL_SD_CardStateTypeDef state;

    while ((HAL_GetTick() - tick) < timeout_ms) {
        state = HAL_SD_GetCardState(&hsd);
        if (state == HAL_SD_CARD_TRANSFER) {
            return 1;
        }
        if (state == HAL_SD_CARD_ERROR) {
            diskio_log("[DISKIO] wait: CARD ERROR\r\n");
            return 0;
        }
        IWDG->KR = 0xAAAA;
    }

    diskio_log("[DISKIO] wait: TIMEOUT state=%d err=0x%08lX\r\n",
               (int)state, HAL_SD_GetError(&hsd));
    return 0;
}

DSTATUS USER_initialize(BYTE pdrv)
{
    HAL_SD_CardInfoTypeDef info;

    if (pdrv != 0) {
        diskio_log("[DISKIO] init: wrong pdrv=%u\r\n", pdrv);
        return STA_NOINIT;
    }

    Stat = STA_NOINIT;
    diskio_log("[DISKIO] init: begin\r\n");

    /* 🔴 Инициализация SDIO здесь, а не в MX_SDIO_SD_Init */
    if (HAL_SD_Init(&hsd) != HAL_OK) {
        diskio_log("[DISKIO] init: HAL_SD_Init failed\r\n");
        return STA_NOINIT;
    }

    /* Ждём готовности */
    if (!sd_wait_ready(SD_READY_WAIT_MS)) {
        diskio_log("[DISKIO] init: card not ready\r\n");
        return Stat;
    }

    HAL_SD_CardStateTypeDef state = HAL_SD_GetCardState(&hsd);
    diskio_log("[DISKIO] init: ready cardState=%d\r\n", (int)state);

    if (HAL_SD_GetCardInfo(&hsd, &info) == HAL_OK) {
        diskio_log("[DISKIO] init: blocks=%lu blockSize=%lu\r\n",
                   (unsigned long)info.LogBlockNbr,
                   (unsigned long)info.LogBlockSize);
    }

    Stat = 0;
    diskio_log("[DISKIO] init: Stat=0x%02X\r\n", Stat);
    return Stat;
}

DSTATUS USER_status(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;

    HAL_SD_CardStateTypeDef state = HAL_SD_GetCardState(&hsd);
    Stat = (state == HAL_SD_CARD_TRANSFER) ? 0 : STA_NOINIT;

    if (Stat != 0) {
        diskio_log("[DISKIO] status: not ready, state=%d\r\n", (int)state);
    }

    return Stat;
}

DRESULT USER_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    diskio_log("[DISKIO] read: sec=%lu cnt=%u\r\n",
               (unsigned long)sector, (unsigned)count);

    if (pdrv != 0 || buff == NULL || count == 0) return RES_PARERR;
    if (USER_status(pdrv) & STA_NOINIT) {
        diskio_log("[DISKIO] read: not ready\r\n");
        return RES_NOTRDY;
    }

    if (!sd_wait_ready(SD_READY_WAIT_MS)) {
        diskio_log("[DISKIO] read: card not ready before read\r\n");
        return RES_ERROR;
    }

    HAL_StatusTypeDef hs = HAL_SD_ReadBlocks(
        &hsd, buff, (uint32_t)sector, (uint32_t)count, SD_TIMEOUT_MS);

    if (hs != HAL_OK) {
        diskio_log("[DISKIO] read poll err=0x%08lX hs=%d\r\n",
                   HAL_SD_GetError(&hsd), (int)hs);
        return RES_ERROR;
    }

    if (!sd_wait_ready(SD_TIMEOUT_MS)) {
        diskio_log("[DISKIO] read: ready wait after read failed\r\n");
        return RES_ERROR;
    }

    diskio_log("[DISKIO] read: OK\r\n");
    return RES_OK;
}

#if _USE_WRITE == 1
DRESULT USER_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    diskio_log("[DISKIO] write: CORRECT FILE v2.4 SINGLE\r\n");
    diskio_log("[DISKIO] write: sec=%lu cnt=%u\r\n",
               (unsigned long)sector, (unsigned)count);

    if (pdrv != 0 || buff == NULL || count == 0) return RES_PARERR;
    if (USER_status(pdrv) & STA_NOINIT) {
        diskio_log("[DISKIO] write: not ready\r\n");
        return RES_NOTRDY;
    }

    /* Ждём готовности */
    if (!sd_wait_ready(SD_READY_WAIT_MS)) {
        HAL_SD_CardStateTypeDef state = HAL_SD_GetCardState(&hsd);
        diskio_log("[DISKIO] write: card not ready, state=%d\r\n", (int)state);
        return RES_ERROR;
    }

    __disable_irq();
    HAL_StatusTypeDef hs = HAL_SD_WriteBlocks(
        &hsd, (uint8_t*)buff, (uint32_t)sector, (uint32_t)count, SD_TIMEOUT_MS);
    __enable_irq();

    if (hs != HAL_OK) {
        diskio_log("[DISKIO] write poll err=0x%08lX hs=%d\r\n",
                   HAL_SD_GetError(&hsd), (int)hs);
        return RES_ERROR;
    }

    if (!sd_wait_ready(SD_TIMEOUT_MS)) {
        HAL_SD_CardStateTypeDef state = HAL_SD_GetCardState(&hsd);
        diskio_log("[DISKIO] write: ready wait failed, state=%d\r\n", (int)state);
        return RES_ERROR;
    }

    HAL_Delay(5);
    diskio_log("[DISKIO] write: OK\r\n");
    return RES_OK;
}
#endif

#if _USE_IOCTL == 1
DRESULT USER_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    HAL_SD_CardInfoTypeDef info;
    DRESULT res = RES_ERROR;

    if (pdrv != 0) return RES_PARERR;

    switch (cmd) {
    case CTRL_SYNC:
        res = sd_wait_ready(SD_TIMEOUT_MS) ? RES_OK : RES_ERROR;
        break;

    case GET_SECTOR_COUNT:
        if (USER_status(pdrv) & STA_NOINIT) return RES_NOTRDY;
        if (HAL_SD_GetCardInfo(&hsd, &info) == HAL_OK) {
            *(DWORD*)buff = info.LogBlockNbr;
            res = RES_OK;
        }
        break;

    case GET_SECTOR_SIZE:
        if (USER_status(pdrv) & STA_NOINIT) return RES_NOTRDY;
        *(WORD*)buff = 512;
        res = RES_OK;
        break;

    case GET_BLOCK_SIZE:
        if (USER_status(pdrv) & STA_NOINIT) return RES_NOTRDY;
        if (HAL_SD_GetCardInfo(&hsd, &info) == HAL_OK) {
            *(DWORD*)buff = 1;
            res = RES_OK;
        }
        break;

    default:
        res = RES_PARERR;
        break;
    }

    return res;
}
#endif

/**
 * @file  sdio.c
 * @brief SDIO для SD-карты через HAL_SD (без DMA, без IRQ).
 *
 * ClockDiv=118  →  SDIO_CK = 48 MHz / (118+2) = 400 kHz  — фаза идентификации.
 * После успешной инициализации sd_diskio.c переключает ClockDiv=0
 * (рабочая скорость 24 МГц для 1-bit шины).
 */

#include "sdio.h"
#include "stm32f4xx_hal_sd.h"
#include "main.h"

SD_HandleTypeDef hsd;

void MX_SDIO_SD_Init(void)
{
  hsd.Instance                 = SDIO;
  hsd.Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide             = SDIO_BUS_WIDE_1B;
<<<<<<< HEAD
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_ENABLE;
=======
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
>>>>>>> e0288c31b99ce296d5e80e4e68e4796860bf38b1
  /*
   * ClockDiv=118: SDIO_CK = SDIOCLK(48 MHz) / (118+2) = 400 kHz.
   * This is the SD card identification speed limit (max 400 kHz).
   * SD_initialize() in sd_diskio.c will raise the speed to 24 MHz
   * (ClockDiv=0) after successful init.
   */
<<<<<<< HEAD
  hsd.Init.ClockDiv            = 40U;
=======
  hsd.Init.ClockDiv            = 118U;
>>>>>>> e0288c31b99ce296d5e80e4e68e4796860bf38b1

  if (HAL_SD_Init(&hsd) != HAL_OK)
  {
    Error_Handler();
  }
  /* Stay on 1-bit bus — no D1..D3 pins wired in HAL_SD_MspInit */
<<<<<<< HEAD

=======
  if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_1B) != HAL_OK)
  {
    Error_Handler();
  }
>>>>>>> e0288c31b99ce296d5e80e4e68e4796860bf38b1
}

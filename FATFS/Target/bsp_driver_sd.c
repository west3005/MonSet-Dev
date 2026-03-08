/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    bsp_driver_sd.c for F4
  * @brief   Generic uSD card driver for FatFs
  *          Aligned with Core/Src/sdio.c where SDIO is already initialized.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifdef OLD_API

#else

#include "bsp_driver_sd.h"

extern SD_HandleTypeDef hsd;

/**
  * @brief  Initializes the SD card device.
  * @retval SD status
  */
__weak uint8_t BSP_SD_Init(void)
{
  if (BSP_SD_IsDetected() != SD_PRESENT) {
    return MSD_ERROR;
  }

  /* SDIO already initialized in MX_SDIO_SD_Init() */
  return MSD_OK;
}

/**
  * @brief  Configures Interrupt mode for SD detection pin.
  * @retval Returns 0
  */
__weak uint8_t BSP_SD_ITConfig(void)
{
  return (uint8_t)0;
}

/**
  * @brief  SD detect IT treatment
  */
__weak void BSP_SD_DetectIT(void)
{
}

/**
  * @brief  Reads block(s) from a specified address in an SD card, in polling mode.
  * @param  pData: Pointer to data buffer
  * @param  ReadAddr: Block address
  * @param  NumOfBlocks: Number of blocks
  * @param  Timeout: Operation timeout
  * @retval SD status
  */
__weak uint8_t BSP_SD_ReadBlocks(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{
  return (HAL_SD_ReadBlocks(&hsd, (uint8_t *)pData, ReadAddr, NumOfBlocks, Timeout) == HAL_OK)
           ? MSD_OK
           : MSD_ERROR;
}

/**
  * @brief  Writes block(s) to a specified address in an SD card, in polling mode.
  * @param  pData: Pointer to data buffer
  * @param  WriteAddr: Block address
  * @param  NumOfBlocks: Number of blocks
  * @param  Timeout: Operation timeout
  * @retval SD status
  */
__weak uint8_t BSP_SD_WriteBlocks(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{
  return (HAL_SD_WriteBlocks(&hsd, (uint8_t *)pData, WriteAddr, NumOfBlocks, Timeout) == HAL_OK)
           ? MSD_OK
           : MSD_ERROR;
}

/**
  * @brief  Reads block(s) in DMA mode.
  */
__weak uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks)
{
  return (HAL_SD_ReadBlocks_DMA(&hsd, (uint8_t *)pData, ReadAddr, NumOfBlocks) == HAL_OK)
           ? MSD_OK
           : MSD_ERROR;
}

/**
  * @brief  Writes block(s) in DMA mode.
  */
__weak uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks)
{
  return (HAL_SD_WriteBlocks_DMA(&hsd, (uint8_t *)pData, WriteAddr, NumOfBlocks) == HAL_OK)
           ? MSD_OK
           : MSD_ERROR;
}

/**
  * @brief  Erases SD area.
  */
__weak uint8_t BSP_SD_Erase(uint32_t StartAddr, uint32_t EndAddr)
{
  return (HAL_SD_Erase(&hsd, StartAddr, EndAddr) == HAL_OK)
           ? MSD_OK
           : MSD_ERROR;
}

/**
  * @brief  Gets SD card transfer state.
  */
__weak uint8_t BSP_SD_GetCardState(void)
{
  return (HAL_SD_GetCardState(&hsd) == HAL_SD_CARD_TRANSFER)
           ? SD_TRANSFER_OK
           : SD_TRANSFER_BUSY;
}

/**
  * @brief  Gets SD card info.
  */
__weak void BSP_SD_GetCardInfo(HAL_SD_CardInfoTypeDef *CardInfo)
{
  HAL_SD_GetCardInfo(&hsd, CardInfo);
}

void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
{
  (void)hsd;
  BSP_SD_AbortCallback();
}

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  (void)hsd;
  BSP_SD_WriteCpltCallback();
}

void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
  (void)hsd;
  BSP_SD_ReadCpltCallback();
}

__weak void BSP_SD_AbortCallback(void)
{
}

__weak void BSP_SD_WriteCpltCallback(void)
{
}

__weak void BSP_SD_ReadCpltCallback(void)
{
}

#endif

/**
  * @brief  Detects if SD card is plugged in.
  * @retval SD presence status
  */
__weak uint8_t BSP_SD_IsDetected(void)
{
  __IO uint8_t status = SD_PRESENT;
  return status;
}

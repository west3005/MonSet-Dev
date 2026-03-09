/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    fatfs.h
  * @brief   Header for fatfs applications
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __FATFS_H
#define __FATFS_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "ff.h"
#include "ff_gen_drv.h"
#include "user_diskio.h"  /* defines USER_Driver as external */

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern uint8_t retUSER;    /* Return value for USER */
extern char USERPath[4];   /* USER logical drive path */
extern FATFS USERFatFS;    /* File system object for USER logical drive */
extern FIL USERFile;       /* File object for USER */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_FATFS_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __FATFS_H */

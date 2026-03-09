/**
 * @file user_diskio.h
 * @brief User diskio driver header
 */

#ifndef __USER_DISKIO_H
#define __USER_DISKIO_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "ff_gen_drv.h"  // ← ДОБАВЛЕНО: для Diskio_drvTypeDef

/* Exported types --------------------------------------------------------*/
/* Exported constants --------------------------------------------------*/
/* Exported functions --------------------------------------------------*/

extern Diskio_drvTypeDef USER_Driver;

#ifdef __cplusplus
}
#endif

#endif /* __USER_DISKIO_H */

/* add user code begin Header */
/**
  **************************************************************************
  * @file     wk_i2c_app.h
  * @brief    i2c application config header file
  **************************************************************************
  *
  * Copyright (c) 2025, Artery Technology, All rights reserved.
  * SPDX-License-Identifier: BSD-3-Clause
  *
  **************************************************************************
  */
/* add user code end Header */

/* define to prevent recursive inclusion -------------------------------------*/
#ifndef __I2C_APP_H
#define __I2C_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "i2c_application.h"

#ifndef I2C_MEM_ADDR_WIDTH_8
#define I2C_MEM_ADDR_WIDTH_8  I2C_MEM_ADDR_WIDIH_8
#endif
#ifndef I2C_MEM_ADDR_WIDTH_16
#define I2C_MEM_ADDR_WIDTH_16  I2C_MEM_ADDR_WIDIH_16
#endif

/* private includes -------------------------------------------------------------*/
/* add user code begin private includes */

/* add user code end private includes */

/* private define ------------------------------------------------------------*/
/* add user code begin private define */

/* add user code end private define */

/* exported types -------------------------------------------------------------*/
/* add user code begin exported types */

/* add user code end exported types */

/* exported constants --------------------------------------------------------*/
/* add user code begin exported constants */

/* add user code end exported constants */

/* exported macro ------------------------------------------------------------*/
/* add user code begin exported macro */

/* add user code end exported macro */

void wk_i2c_app_init(void);

void wk_i2c_app_task(void);

/* add user code begin exported functions */

/* add user code end exported functions */

#ifdef __cplusplus
}
#endif

#endif

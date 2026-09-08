/**
  **************************************************************************
  * @file     soft_i2c.h
  * @version  v3.0.0
  * @date     2025-03-?? (modified)
  * @brief    Software I2C (bit-banging) driver header – multi?instance version
  **************************************************************************
  */
#ifndef __SOFT_I2C_H
#define __SOFT_I2C_H

#include "at32f403a_407.h"
#include "perf_counter.h"
#include <stdint.h>
#include <stdio.h>   // for printf() in i2c_check_addr

/*==============================================================================
 *                              I2C 实例结构体
 *============================================================================*/

/**
 * @brief 软件 I2C 实例，包含引脚配置和时钟使能信息
 */
typedef struct {
    gpio_type *scl_port;      // SCL 端口 (例如 GPIOB)
    uint32_t   scl_pin;       // SCL 引脚掩码 (例如 GPIO_PINS_10)
    uint32_t   scl_clk;       // SCL 端口时钟宏 (例如 CRM_GPIOB_PERIPH_CLOCK)

    gpio_type *sda_port;      // SDA 端口
    uint32_t   sda_pin;       // SDA 引脚掩码
    uint32_t   sda_clk;       // SDA 端口时钟宏
} soft_i2c_t;

/* 超时定义（从机应答等待次数）*/
#define I2C_ACK_TIMEOUT  50

/*==============================================================================
 *                              API 函数声明
 *============================================================================*/

void soft_i2c_init(soft_i2c_t *i2c,
                   gpio_type *scl_port, uint32_t scl_pin, uint32_t scl_clk,
                   gpio_type *sda_port, uint32_t sda_pin, uint32_t sda_clk);

void soft_i2c_start(soft_i2c_t *i2c);
void soft_i2c_stop(soft_i2c_t *i2c);
void soft_i2c_send_byte(soft_i2c_t *i2c, uint8_t data);
uint8_t soft_i2c_receive_byte(soft_i2c_t *i2c);
uint8_t soft_i2c_wait_ack(soft_i2c_t *i2c, uint8_t timeout);
void soft_i2c_ack(soft_i2c_t *i2c);
void soft_i2c_no_ack(soft_i2c_t *i2c);
void soft_i2c_check_addr(soft_i2c_t *i2c);

/* 高层辅助函数 */
void soft_i2c_write_cmd(soft_i2c_t *i2c, uint8_t command, uint8_t device_addr, uint8_t register_addr);
void soft_i2c_write_dat(soft_i2c_t *i2c, uint8_t data, uint8_t device_addr, uint8_t register_addr);

#endif /* __SOFT_I2C_H */
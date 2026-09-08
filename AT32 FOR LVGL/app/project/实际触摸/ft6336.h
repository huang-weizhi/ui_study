#ifndef __FT6336_H
#define __FT6336_H
 
#include <stdlib.h>
#include <stdio.h>

#include "soft_i2c.h"
 
#define FT_INT_PIN                  GPIO_PINS_15
#define FT_INT_GPIO_PORT             GPIOB
#define FT_INT_GPIO_CLK              CRM_GPIOB_PERIPH_CLOCK

#define FT_RST_PIN                   GPIO_PINS_7
#define FT_RST_GPIO_PORT             GPIOA
#define FT_RST_GPIO_CLK              CRM_GPIOA_PERIPH_CLOCK

#define FT_TRUE                  1
#define FT_FALSE                 0
 
//I2C读写命令
#define FT_CMD_WR                0X70         //写命令
#define FT_CMD_RD                0X71         //读命令
  
//FT5426 部分寄存器定义 
#define FT_DEVIDE_MODE           0x00         //FT6336模式控制寄存器
#define FT_REG_NUM_FINGER        0x02         //触摸状态寄存器
 
#define FT_TP1_REG               0X03         //第一个触摸点数据地址
#define FT_TP2_REG               0X09         //第二个触摸点数据地址
 
#define FT_ID_G_CIPHER_MID       0x9F         //芯片代号（中字节） 默认值0x26
#define FT_ID_G_CIPHER_LOW       0xA0         //芯片代号（低字节） 0x01: Ft6336G  0x02: Ft6336U 
#define FT_ID_G_LIB_VERSION      0xA1         //版本
#define FT_ID_G_CIPHER_HIGH      0xA3         //芯片代号（高字节） 默认0x64 
#define FT_ID_G_MODE             0xA4         //FT6636中断模式控制寄存器
#define FT_ID_G_FOCALTECH_ID     0xA8         //VENDOR ID 默认值为0x11
#define FT_ID_G_THGROUP          0x80         //触摸有效值设置寄存器
#define FT_ID_G_PERIODACTIVE     0x88         //激活状态周期设置寄存器
 
 
// MATCH VALUE LIST 
#define PANNEL_ID                0x11

// 屏幕分辨率（横屏320×480）
#define TOUCH_SCREEN_WIDTH       480     // 注意：横屏时宽度是480
#define TOUCH_SCREEN_HEIGHT      320     // 横屏时高度是320

// 触摸点数据结构
typedef struct {
    uint8_t id;          // 触摸点ID
    uint16_t x;          // X坐标（原始值）
    uint16_t y;          // Y坐标（原始值）
    uint8_t event;       // 触摸事件 (0:按下, 1:抬起, 2:接触)
    uint8_t weight;      // 触摸压力
    uint8_t area;        // 触摸面积
} TouchPoint_t;

// 触摸状态
typedef struct {
    uint8_t touch_count;      // 当前触摸点数量
    TouchPoint_t points[2];   // 最多支持2点触摸
} TouchData_t;
 
 
int ft6336_Init( void );
int ft6336_rd_register(uint16_t reg, uint8_t *buff, uint8_t len);
int ft6336_rd_sigleReg(uint16_t reg, uint8_t *val );

 
#endif    /* __FT6336_H */
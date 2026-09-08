/**
  ******************************************************************************
  *                              (c) Copyright 2018, tangmingfei2013@126.com
  *                                      All Rights Reserved
  * @file    iic.c
  * @author  mingfei tang
  * @version V1.0.0
  * @date    2018/03/25
  * @description:
    This source code and any compilation or derivative thereof is the proprietary
    information of mingfei.tang and is confidential in nature.
    Under no circumstances is this software to be combined with any
    Open Source Software in any way or placed under an Open Source License
    of any type without the express written permission of mingfei.tang
  ******************************************************************************
**/
/* Includes ------------------------------------------------------------------*/
#include "ft6336.h"
#include "perf_counter.h"
#define FT_RST_L     gpio_bits_reset(FT_RST_GPIO_PORT, FT_RST_PIN)
#define FT_RST_H     gpio_bits_set(FT_RST_GPIO_PORT, FT_RST_PIN)
 
static void touch_delay_us(uint32_t us)
{
   delay_us(us);
}
 


int i2c_wr_register(uint16_t reg, uint8_t *buff, uint8_t len)
{
    int i;
    int set = FT_FALSE;
    
    i2c_start();
    i2c_send_byte( FT_CMD_WR );
    if (i2c_wait_ack(I2C_ACK_TIMEOUT) != 0)
    {
        goto cmd_fail;
    }
    
    i2c_send_byte( reg&0XFF );
    if (i2c_wait_ack(I2C_ACK_TIMEOUT) != 0)
    {
        goto cmd_fail;
    }
    
    // send data 
    for(i=0;i<len;i++)
    {
        i2c_send_byte( buff[i] );
        if (i2c_wait_ack(I2C_ACK_TIMEOUT) != 0)
        {
            goto cmd_fail;
        }
    }
    
    set = FT_TRUE;
    i2c_stop();
    
    return set;
    
cmd_fail: 
    i2c_stop();
    
    return set;
}
 
int ft6336_rd_register(uint16_t reg, uint8_t *buff, uint8_t len)
{
    int i;
    int set = FT_TRUE;
    
    i2c_start();
    i2c_send_byte( FT_CMD_WR );
    if (i2c_wait_ack(I2C_ACK_TIMEOUT) != 0)
    {
        goto cmd_fail;
    }
    
    i2c_send_byte( reg&0XFF );
    if (i2c_wait_ack(I2C_ACK_TIMEOUT) != 0)
    {
        goto cmd_fail;
    }
    
    // start to read data from rigister 
    i2c_start();
    // send command
    i2c_send_byte( FT_CMD_RD );
    if (i2c_wait_ack(I2C_ACK_TIMEOUT) != 0)
    {
        goto cmd_fail;
    }
    
    for(i=0;i<len;i++)
    {   
        buff[i]= i2c_receive_byte();
        i2c_no_ack();
    }
    
    i2c_stop();
    return set;
    
cmd_fail: 
    i2c_stop();
    set = FT_FALSE;
    
    return set;
}
 
int ft6336_rd_sigleReg(uint16_t reg, uint8_t *val )
{
    int set = FT_TRUE;
    
    i2c_start();
    i2c_send_byte( FT_CMD_WR );
    if (i2c_wait_ack(I2C_ACK_TIMEOUT) != 0)
    {
        goto cmd_fail;
    }
    
    i2c_send_byte( reg&0XFF );
    if (i2c_wait_ack(I2C_ACK_TIMEOUT) != 0)
    {
        goto cmd_fail;
    }
    
    // start to read data from rigister 
    i2c_start();
    // send command
    i2c_send_byte( FT_CMD_RD );
    if (i2c_wait_ack(I2C_ACK_TIMEOUT) != 0)
    {
        goto cmd_fail;
    }
     
    *val = i2c_receive_byte();
    i2c_no_ack();
    
    i2c_stop();
    return set;
    
cmd_fail: 
    i2c_stop();
    set = FT_FALSE;
    
    return set;
}
 
static void  ft6336_rest( void )
{
    FT_RST_L;
    touch_delay_us( 10 );
    FT_RST_H;
    touch_delay_us( 50000 );
}

/**
  * @brief  初始化FT6336触摸芯片
  * @retval FT_TRUE: 成功, FT_FALSE: 失败
  */
int ft6336_Init( void )
{
    int set = FT_FALSE;
    uint8_t id;
    
    ft6336_rest();
    
    // 等待芯片稳定
    touch_delay_us(200000);  // 200ms
    
    // 读取芯片ID 
    ft6336_rd_sigleReg(FT_ID_G_FOCALTECH_ID,&id);
    if(id != PANNEL_ID)
    {
        return set;
    }
    set = FT_TRUE;
    
    // 可选：配置触摸芯片参数
    uint8_t config_data;
    
    // 设置触摸阈值（灵敏度）
    config_data = 0x08;
    i2c_wr_register(FT_ID_G_THGROUP, &config_data, 1);
    
    // 设置激活周期（上报频率）
    config_data = 0x06;  // 6ms
    i2c_wr_register(FT_ID_G_PERIODACTIVE, &config_data, 1);
    
    // 设置中断模式为中断模式
    config_data = 0x01;
    i2c_wr_register(FT_ID_G_MODE, &config_data, 1);
    
    // 设置工作模式为正常模式
    config_data = 0x00;
    i2c_wr_register(FT_DEVIDE_MODE, &config_data, 1);
    
    return set;
}
 
/*End of this file */
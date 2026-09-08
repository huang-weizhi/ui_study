/**
  **************************************************************************
  * @file     soft_i2c.c
  * @version  v3.0.0
  * @date     2025-03-?? (modified)
  * @brief    Software I2C (bit-banging) driver – multi‑instance version
  *
  * @details  All timing is based on CPU cycle counting using delay_us().
  *           Supports multiple I2C buses via instance structure.
  **************************************************************************
  */
#include "soft_i2c.h"

/*==============================================================================
 *                           内部辅助宏 / 内联函数
 *============================================================================*/

#define I2C_SCL_HIGH(i2c)   gpio_bits_set((i2c)->scl_port, (i2c)->scl_pin)
#define I2C_SCL_LOW(i2c)    gpio_bits_reset((i2c)->scl_port, (i2c)->scl_pin)
#define I2C_SDA_HIGH(i2c)   gpio_bits_set((i2c)->sda_port, (i2c)->sda_pin)
#define I2C_SDA_LOW(i2c)    gpio_bits_reset((i2c)->sda_port, (i2c)->sda_pin)
#define I2C_SDA_READ(i2c)   (gpio_input_data_bit_read((i2c)->sda_port, (i2c)->sda_pin) == SET)

static void SDA_OUT(soft_i2c_t *i2c)
{
    gpio_init_type gpio_initstructure;
    gpio_initstructure.gpio_out_type     = GPIO_OUTPUT_OPEN_DRAIN;
    gpio_initstructure.gpio_pull         = GPIO_PULL_NONE;
    gpio_initstructure.gpio_mode         = GPIO_MODE_OUTPUT;
    gpio_initstructure.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_initstructure.gpio_pins         = i2c->sda_pin;
    gpio_init(i2c->sda_port, &gpio_initstructure);
}

static void SDA_IN(soft_i2c_t *i2c)
{
    gpio_init_type gpio_initstructure;
    gpio_initstructure.gpio_out_type     = GPIO_OUTPUT_OPEN_DRAIN;
    gpio_initstructure.gpio_pull         = GPIO_PULL_UP;   /* was NONE: SDA floated when the slave released it, sampling garbage at -O0 */
    gpio_initstructure.gpio_mode         = GPIO_MODE_INPUT;
    gpio_initstructure.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_initstructure.gpio_pins         = i2c->sda_pin;
    gpio_init(i2c->sda_port, &gpio_initstructure);
}

static void i2c_delay(void)
{
    delay_us(5);
}

/*==============================================================================
 *                           公共 API 实现
 *============================================================================*/

void soft_i2c_init(soft_i2c_t *i2c,
                   gpio_type *scl_port, uint32_t scl_pin, uint32_t scl_clk,
                   gpio_type *sda_port, uint32_t sda_pin, uint32_t sda_clk)
{
    i2c->scl_port = scl_port;
    i2c->scl_pin  = scl_pin;
    i2c->scl_clk  = scl_clk;
    i2c->sda_port = sda_port;
    i2c->sda_pin  = sda_pin;
    i2c->sda_clk  = sda_clk;

    crm_periph_clock_enable(i2c->scl_clk, TRUE);
    crm_periph_clock_enable(i2c->sda_clk, TRUE);

    gpio_init_type gpio_initstructure;
    gpio_initstructure.gpio_out_type      = GPIO_OUTPUT_PUSH_PULL;
    gpio_initstructure.gpio_pull          = GPIO_PULL_UP;
    gpio_initstructure.gpio_mode          = GPIO_MODE_OUTPUT;
    gpio_initstructure.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;

    gpio_initstructure.gpio_pins = i2c->scl_pin;
    gpio_init(i2c->scl_port, &gpio_initstructure);

    gpio_initstructure.gpio_pins = i2c->sda_pin;
    gpio_init(i2c->sda_port, &gpio_initstructure);

    I2C_SCL_HIGH(i2c);
    I2C_SDA_HIGH(i2c);
}

void soft_i2c_start(soft_i2c_t *i2c)
{
    SDA_OUT(i2c);
    i2c_delay();

    I2C_SDA_HIGH(i2c);
    I2C_SCL_HIGH(i2c);
    i2c_delay();

    I2C_SDA_LOW(i2c);
    i2c_delay();

    I2C_SCL_LOW(i2c);
    i2c_delay();
}

void soft_i2c_stop(soft_i2c_t *i2c)
{
    SDA_OUT(i2c);
    I2C_SDA_LOW(i2c);
    i2c_delay();

    I2C_SCL_HIGH(i2c);
    i2c_delay();

    I2C_SDA_HIGH(i2c);
    i2c_delay();
}

void soft_i2c_send_byte(soft_i2c_t *i2c, uint8_t data)
{
    uint8_t i = 8;
    SDA_OUT(i2c);

    while (i--)
    {
        I2C_SCL_LOW(i2c);

        if (data & 0x80)
            I2C_SDA_HIGH(i2c);
        else
            I2C_SDA_LOW(i2c);

        i2c_delay();
        data <<= 1;

        I2C_SCL_HIGH(i2c);
        i2c_delay();
    }

    I2C_SCL_LOW(i2c);
    I2C_SDA_HIGH(i2c);
}

uint8_t soft_i2c_receive_byte(soft_i2c_t *i2c)
{
    uint8_t i = 8;
    uint8_t byte = 0;

    SDA_IN(i2c);
    I2C_SDA_HIGH(i2c);

    while (i--)
    {
        byte <<= 1;

        I2C_SCL_LOW(i2c);
        i2c_delay();

        I2C_SCL_HIGH(i2c);
        i2c_delay();

        if (I2C_SDA_READ(i2c))
            byte |= 0x01;
    }

    I2C_SCL_LOW(i2c);
    return byte;
}

uint8_t soft_i2c_wait_ack(soft_i2c_t *i2c, uint8_t timeout)
{
    SDA_IN(i2c);
    I2C_SCL_LOW(i2c);
    I2C_SDA_HIGH(i2c);
    i2c_delay();

    while (timeout--)
    {
        if (I2C_SDA_READ(i2c) == 0)
        {
            I2C_SCL_HIGH(i2c);
            i2c_delay();
            I2C_SCL_LOW(i2c);
            return 0;
        }
        i2c_delay();
    }

    I2C_SCL_HIGH(i2c);
    i2c_delay();
    I2C_SCL_LOW(i2c);
    return 1;
}

void soft_i2c_ack(soft_i2c_t *i2c)
{
    SDA_OUT(i2c);
    I2C_SDA_LOW(i2c);
    i2c_delay();

    I2C_SCL_HIGH(i2c);
    i2c_delay();

    I2C_SCL_LOW(i2c);
    i2c_delay();

    I2C_SDA_HIGH(i2c);
    i2c_delay();
}

void soft_i2c_no_ack(soft_i2c_t *i2c)
{
    SDA_OUT(i2c);
    I2C_SDA_HIGH(i2c);
    i2c_delay();

    I2C_SCL_HIGH(i2c);
    i2c_delay();

    I2C_SCL_LOW(i2c);
    i2c_delay();
}

void soft_i2c_write_cmd(soft_i2c_t *i2c, uint8_t command, uint8_t device_addr, uint8_t register_addr)
{
    uint8_t addr_w = (device_addr << 1) | 0x00;

    soft_i2c_start(i2c);
    soft_i2c_send_byte(i2c, addr_w);
    soft_i2c_wait_ack(i2c, I2C_ACK_TIMEOUT);
    soft_i2c_send_byte(i2c, register_addr);
    soft_i2c_wait_ack(i2c, I2C_ACK_TIMEOUT);
    soft_i2c_send_byte(i2c, command);
    soft_i2c_wait_ack(i2c, I2C_ACK_TIMEOUT);
    soft_i2c_stop(i2c);
}

void soft_i2c_write_dat(soft_i2c_t *i2c, uint8_t data, uint8_t device_addr, uint8_t register_addr)
{
    uint8_t addr_w = (device_addr << 1) | 0x00;

    soft_i2c_start(i2c);
    soft_i2c_send_byte(i2c, addr_w);
    soft_i2c_wait_ack(i2c, I2C_ACK_TIMEOUT);
    soft_i2c_send_byte(i2c, register_addr);
    soft_i2c_wait_ack(i2c, I2C_ACK_TIMEOUT);
    soft_i2c_send_byte(i2c, data);
    soft_i2c_wait_ack(i2c, I2C_ACK_TIMEOUT);
    soft_i2c_stop(i2c);
}

void soft_i2c_check_addr(soft_i2c_t *i2c)
{
    uint8_t found = 0;

    for (uint8_t addr = 0x00; addr < 0xFF; addr++)
    {
        uint8_t addr_w = (addr << 1) | 0x00;

        soft_i2c_start(i2c);
        soft_i2c_send_byte(i2c, addr_w);
        if (soft_i2c_wait_ack(i2c, I2C_ACK_TIMEOUT) == 0)
        {
            printf("Device found at 0x%02X\r\n", addr);
            found = 1;
        }
        soft_i2c_stop(i2c);
        delay_us(5000);
    }

    if (!found)
        printf("No I2C device found on this bus!\r\n");
}
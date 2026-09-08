/*=============================================================================
 * pc_handle.c - PC communication protocol handler
 *
 * Implements the UART frame protocol between the device and the PC tool:
 *   frame: 0xD1 0xF0 | cmd1 | cmd2 | id(2B) | data-len(2B) ... | CRC32
 * cmd1 = GENERAL (register read/write) or FIRMWARE_UPDATE.
 * Firmware update flow: CONNECT -> ACK -> START -> receive 1KB packets that
 * are written to the external SPI flash -> END, then reboot into IAP which
 * copies the image from SPI flash to internal flash.
 * pc_comm_handle() is the periodic entry point (PC_HANDLE_TIME ms).
 *===========================================================================*/#include <stdbool.h>
#include <string.h>
#include "pc_handle.h"

 

PC_DATA_t pc_data;
pc_event_e pc_event = PC_END_EVENT;
static uint32_t current_flash_addr = SPI_FLASH_UPDATE_START_ADDR;


uint8_t *tx_buffer = NULL;
uint16_t tx_remain = 0;


extern QueueHandle_t usart_1_rx_handle;


static void RecvDataFromPC(PC_DATA_t *pc);
static void WriteDataToPC(PC_DATA_t *pc,
                          uint16_t cmd_1,  uint16_t cmd_2,
                          uint16_t id,     uint16_t data_len,
                          uint16_t data_1, uint16_t data_2,
                          uint16_t data_3, uint16_t data_4,
                          uint16_t data_5);
void pc_event_handle(void);
static void RecvFlashDataFromPC(PC_DATA_t *pc);
void flash_ask_ok(PC_DATA_t *pc);
void flash_ask_error(PC_DATA_t *pc);
 
//void send_heartbeat_data(PC_DATA_t *pc, HA01_t *this);


static int parse_version_string(const char *str, uint8_t *major, uint8_t *minor, uint8_t *patch);
uint32_t GetMcuInfo(void);


static uint8_t sw_ver_major, sw_ver_minor, sw_ver_patch;
static uint8_t hw_ver_major, hw_ver_minor, hw_ver_patch;

void init_version_parse(void)
{
    parse_version_string(SOFTWARE_VERSTION, &sw_ver_major, &sw_ver_minor, &sw_ver_patch);
    parse_version_string(HARDWARE_VERSTION, &hw_ver_major, &hw_ver_minor, &hw_ver_patch);
}




uint32_t convert_data(const uint8_t *input_array, uint32_t *output_array,
                      uint32_t start_index, uint32_t end_index)
{
    if (input_array == NULL || output_array == NULL)
    {
        return 0;
    }

    if (start_index > end_index)
    {
        return 0;
    }

    uint32_t byte_count = end_index - start_index + 1;
    const uint8_t *ptr = &input_array[start_index];
    uint32_t i;
    uint32_t output_index = 0;

    for (i = 0; i < byte_count / 4; i++)
    {
        output_array[output_index++] = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
        ptr += 4;
    }

    if (byte_count % 4 != 0)
    {
        uint32_t word = 0;
        uint8_t remaining = byte_count % 4;

        for (uint8_t j = 0; j < remaining; j++)
        {
            word |= ptr[j] << (24 - (j * 8));
        }

        output_array[output_index++] = word;
    }

    return output_index;
}


uint16_t usart_receiveData(uint8_t usart_num, uint8_t *buffer)
{
    if (usart_num != 0 || buffer == NULL) return 0;  

    uint16_t count = 0;
    
    for (uint16_t i = 0; i < PC_RX_BUFF_SIZE; i++)
    {
        if (xQueueReceive(usart_1_rx_handle, &buffer[i], pdMS_TO_TICKS(100)) == pdTRUE)
        {
            count++;
        }
        else
        {
            
            break;
        }
    }
    return count;
}

uint16_t usart_sendData(uint8_t usart_num, uint8_t *buffer, uint16_t size)
{
    if (usart_num != 0 || buffer == NULL || size == 0) return 0;

    
    while (tx_remain != 0)
    {
        taskYIELD();  
    }

    
    tx_buffer = buffer;
    tx_remain = size;

    
    usart_interrupt_enable(USART1, USART_TDC_INT, TRUE);

    
    usart_data_transmit(USART1, *tx_buffer);
    tx_buffer++;
    tx_remain--;

    return size;
}

 
void pc_comm_handle(void)
{
    static TickType_t last_event_tick = 0;
    static TickType_t last_heartbeat_tick = 0;
    TickType_t current_tick;

    current_tick = xTaskGetTickCount();

     
    if ((current_tick - last_event_tick) >= pdMS_TO_TICKS(5 * PC_HANDLE_TIME))
    {
        pc_event_handle();
        last_event_tick = current_tick;
    }

     
    if ((current_tick - last_heartbeat_tick) >= pdMS_TO_TICKS(100 * PC_HANDLE_TIME))
    {
        //send_heartbeat_data(&pc_data, &sha01);
        last_heartbeat_tick = current_tick;
    }

    RecvDataFromPC(&pc_data);
	RecvFlashDataFromPC(&pc_data);
}

static void RecvDataFromPC(PC_DATA_t *pc)
{
	if (spa45.flash_updata)
    {
        return;
    }
    uint32_t crc_value;
    pc->read_size = usart_receiveData(PC_USART, pc->rx_buff);

    if (pc->read_size == PC_MAX_RECV_SIZE)
    {
        if (PC_CHECK_HEAD(pc->rx_buff[PC_HEAD1], pc->rx_buff[PC_HEAD2]))
        {
            /* crc check */
            memset(pc->check_crc_buff, 0, PC_CRC_BUFF_SIZE);
            convert_data(pc->rx_buff, pc->check_crc_buff, PC_CMD1, PC_DATA5_LEN_L);
            crc_value = crc_block_calculate(pc->check_crc_buff, 4);
            crc_data_reset();

            if (pc->rx_buff[PC_CRC32_1] == ((crc_value >> 24) & 0xff) &&
                pc->rx_buff[PC_CRC32_2] == ((crc_value >> 16) & 0xff) &&
                pc->rx_buff[PC_CRC32_3] == ((crc_value >> 8)  & 0xff) &&
                pc->rx_buff[PC_CRC32_4] == (crc_value & 0xff))
            {
                /* connect PC */
                if (pc->rx_buff[PC_CMD1] == 0x01 && pc->rx_buff[PC_CMD2] == 0x01 && pc->rx_buff[PC_ID_L] == 0x00)
                {
                    pc_event = CONNECT_PC_EVENT;
                }
                /* jump to bootloader */
                else if (pc->rx_buff[PC_CMD1] == PC_FIRMWARE_UPDATE && pc->rx_buff[PC_CMD2] == PC_FIRMWARE_UPDATE_CONNECT
                         && pc->rx_buff[PC_ID_L] == 0x00)
                {
                    pc_event = IAP_EVENT;
                }
				/* updata flash */
                else if (pc->rx_buff[PC_CMD1] == PC_UI_UPDATE && pc->rx_buff[PC_CMD2] == PC_FIRMWARE_UPDATE_CONNECT
                         && pc->rx_buff[PC_ID_L] == 0x00)
                {
                    pc_event = FLASU_UPDATA_EVENT;
                }
                /* other cmd */
                else
                {
                    __NOP();
                }
            }
        }

        pc->read_size = 0;
    }
}

static void RecvFlashDataFromPC(PC_DATA_t *pc)
{
    if (spa45.flash_updata)
    {
        pc->read_size = usart_receiveData(PC_USART, pc->rx_buff);

        if (pc->read_size != UPDATE_PACKET_SIZE)
        {
            return;
        }

        if (PC_CHECK_HEAD(pc->rx_buff[PC_HEAD1], pc->rx_buff[1035]))
        {
            memset(pc->check_crc_buff, 0, PC_CRC_BUFF_SIZE);
            convert_data(pc->rx_buff, pc->check_crc_buff, 1, 1030);
            uint32_t crc_value = crc_block_calculate(pc->check_crc_buff, 258);
            crc_data_reset();

            if (pc->rx_buff[1031] == ((crc_value >> 24) & 0xff)  &&
                    pc->rx_buff[1032] == ((crc_value >> 16) & 0xff)  &&
                    pc->rx_buff[1033] == ((crc_value >> 8)  & 0xff)   &&
                    pc->rx_buff[1034] == (crc_value & 0xff))
            {
                spiflash_write(&pc_data.rx_buff[UPDATE_DATA_OFFSET],
                               current_flash_addr, UPDATE_DATA_LEN);
                current_flash_addr += 1024;
                flash_ask_ok(&pc_data);

                if (pc->rx_buff[2] == 0x04)
                {
                    vTaskDelay(20);
                    nvic_system_reset();
                }
            }
            else
            {
                flash_ask_error(&pc_data);
            }
        }
        else
        {
            flash_ask_error(&pc_data);
        }

        memset(pc->rx_buff, 0, PC_RX_BUFF_SIZE);
        pc->read_size = 0;
    }
}

static void WriteDataToPC(PC_DATA_t *pc,
                          uint16_t cmd_1,  uint16_t cmd_2,
                          uint16_t id,     uint16_t data_len,
                          uint16_t data_1, uint16_t data_2,
                          uint16_t data_3, uint16_t data_4,
                          uint16_t data_5)
{
    static uint32_t crc_value;
    pc->tx_buff[PC_HEAD1] = PC_HEAD_1;
    pc->tx_buff[PC_CMD1] = cmd_1;
    pc->tx_buff[PC_CMD2] = cmd_2;
    pc->tx_buff[PC_ID_H] = (uint8_t)((id >> 8) & 0xff);
    pc->tx_buff[PC_ID_L] = (uint8_t)((id & 0XFF));
    pc->tx_buff[PC_DATA_LEN_H] = (uint8_t)((data_len >> 8) & 0xff);
    pc->tx_buff[PC_DATA_LEN_L] = (uint8_t)((data_len & 0XFF));
    pc->tx_buff[PC_DATA1_LEN_H] = (uint8_t)((data_1 >> 8) & 0xff);
    pc->tx_buff[PC_DATA1_LEN_L] = (uint8_t)((data_1 & 0XFF));
    pc->tx_buff[PC_DATA2_LEN_H] = (uint8_t)((data_2 >> 8) & 0xff);
    pc->tx_buff[PC_DATA2_LEN_L] = (uint8_t)((data_2 & 0XFF));
    pc->tx_buff[PC_DATA3_LEN_H] = (uint8_t)((data_3 >> 8) & 0xff);
    pc->tx_buff[PC_DATA3_LEN_L] = (uint8_t)((data_3 & 0XFF));
    pc->tx_buff[PC_DATA4_LEN_H] = (uint8_t)((data_4 >> 8) & 0xff);
    pc->tx_buff[PC_DATA4_LEN_L] = (uint8_t)((data_4 & 0XFF));
    pc->tx_buff[PC_DATA5_LEN_H] = (uint8_t)((data_5 >> 8) & 0xff);
    pc->tx_buff[PC_DATA5_LEN_L] = (uint8_t)((data_5 & 0XFF));
    memset(pc->check_crc_buff, 0, PC_CRC_BUFF_SIZE);
    convert_data(pc->tx_buff, pc->check_crc_buff, PC_CMD1, PC_DATA5_LEN_L);
    crc_value = crc_block_calculate(pc->check_crc_buff, 4);
    crc_data_reset();
    pc->tx_buff[PC_CRC32_1] = ((crc_value >> 24) & 0xff);
    pc->tx_buff[PC_CRC32_2] = ((crc_value >> 16) & 0xff);
    pc->tx_buff[PC_CRC32_3] = ((crc_value >> 8) & 0xff);
    pc->tx_buff[PC_CRC32_4] = (crc_value & 0xff);
    pc->tx_buff[PC_HEAD2] = PC_HEAD_2;

    /* send data */
    usart_sendData(PC_USART, pc->tx_buff, PC_MAX_SEND_SIZE);
}

void pc_event_handle(void)
{
    static bool first_in = true;
    static uint32_t crc_value;
    static uint32_t uid_value;

    if (first_in)
    {
        init_version_parse();
        uid_value = GetMcuInfo();
        first_in = false;
    }
	
    switch (pc_event)
    {
    case CONNECT_PC_EVENT:
        pc_data.tx_buff[PC_HEAD1] = PC_HEAD_1;
        pc_data.tx_buff[PC_CMD1] = 0x01;
        pc_data.tx_buff[PC_CMD2] = 0x01;
        pc_data.tx_buff[PC_ID_H] = LOCAL_DEVICE_ID_2;
        pc_data.tx_buff[PC_ID_L] = LOCAL_DEVICE_ID_1;
        pc_data.tx_buff[PC_DATA_LEN_H] = 0x00;
        pc_data.tx_buff[PC_DATA_LEN_L] = 0x0A;
    
	    
        pc_data.tx_buff[PC_DATA1_LEN_H] = sw_ver_major;   
        pc_data.tx_buff[PC_DATA1_LEN_L] = sw_ver_minor;   
        pc_data.tx_buff[PC_DATA2_LEN_H] = sw_ver_patch;   
        
        
        pc_data.tx_buff[PC_DATA2_LEN_L] = hw_ver_major;
        pc_data.tx_buff[PC_DATA3_LEN_H] = hw_ver_minor;
        pc_data.tx_buff[PC_DATA3_LEN_L] = hw_ver_patch;
	
        pc_data.tx_buff[PC_DATA4_LEN_H] = ((uid_value >> 24) & 0xff);
        pc_data.tx_buff[PC_DATA4_LEN_L] = ((uid_value >> 16) & 0xff);
        pc_data.tx_buff[PC_DATA5_LEN_H] = ((uid_value >> 8) & 0xff);
        pc_data.tx_buff[PC_DATA5_LEN_L] = (uid_value & 0xff);

        memset(pc_data.check_crc_buff, 0, PC_CRC_BUFF_SIZE);
        convert_data(pc_data.tx_buff, pc_data.check_crc_buff, PC_CMD1, PC_DATA5_LEN_L);
        crc_value = crc_block_calculate(pc_data.check_crc_buff, 4);
        crc_data_reset();

        pc_data.tx_buff[PC_CRC32_1] = ((crc_value >> 24) & 0xff);
        pc_data.tx_buff[PC_CRC32_2] = ((crc_value >> 16) & 0xff);
        pc_data.tx_buff[PC_CRC32_3] = ((crc_value >> 8) & 0xff);
        pc_data.tx_buff[PC_CRC32_4] = (crc_value & 0xff);
        pc_data.tx_buff[PC_HEAD2] = PC_HEAD_2;

        usart_sendData(PC_USART, pc_data.tx_buff, PC_MAX_SEND_SIZE);
        pc_event = PC_END_EVENT;
        break;

    case IAP_EVENT:
        pc_event = PC_END_EVENT;
        iap_flag = IAP_REV_FLAG_DONE;
        break;

	case FLASU_UPDATA_EVENT:
        spa45.flash_updata = true;
        memset(pc_data.rx_buff, 0, PC_RX_BUFF_SIZE);
        flash_ask_ok(&pc_data);
        pc_event = PC_END_EVENT;
        break;
	
    case PC_END_EVENT:
        break;
    }
}
 

static int parse_version_string(const char *str, uint8_t *major, uint8_t *minor, uint8_t *patch)
{
    if (str == NULL || str[0] != 'V') {
        return -1;
    }
    
    const char *p = str + 1;  
    
    
    *major = 0;
    while (*p >= '0' && *p <= '9') {
        *major = *major * 10 + (*p - '0');
        p++;
    }
    if (*p != '.') return -1;
    p++; 
    
    
    *minor = 0;
    while (*p >= '0' && *p <= '9') {
        *minor = *minor * 10 + (*p - '0');
        p++;
    }
    if (*p != '.') return -1;
    p++; 
    
    
    *patch = 0;
    while (*p >= '0' && *p <= '9') {
        *patch = *patch * 10 + (*p - '0');
        p++;
    }
    
    
    return 0;
}

void flash_ask_ok(PC_DATA_t *pc)
{
    uint32_t crc_value = 0;
    pc->tx_buff[0] = PC_HEAD_1;
    pc->tx_buff[1] = 0x03;
    pc->tx_buff[2] = 0x02;
    pc->tx_buff[3] = LOCAL_DEVICE_ID_2;
    pc->tx_buff[4] = LOCAL_DEVICE_ID_1;
    pc->tx_buff[5] = 0x00;
    pc->tx_buff[6] = 0x0A;
    pc->tx_buff[7] = 0x1D;
    pc->tx_buff[8] = 0x00;
    pc->tx_buff[9] = 0x00;
    pc->tx_buff[10] = 0x00;
    pc->tx_buff[11] = 0x00;
    pc->tx_buff[12] = 0x00;
    pc->tx_buff[13] = 0x00;
    pc->tx_buff[14] = 0x00;
    pc->tx_buff[15] = 0x00;
    pc->tx_buff[16] = 0x00;
    memset(pc->check_crc_buff, 0, PC_CRC_BUFF_SIZE);
    convert_data(pc->tx_buff, pc->check_crc_buff, 1, 16);
    crc_value = crc_block_calculate(pc->check_crc_buff, 4);
    crc_data_reset();
    pc->tx_buff[17] = (crc_value >> 24) & 0xff;
    pc->tx_buff[18] = (crc_value >> 16) & 0xff;
    pc->tx_buff[19] = (crc_value >> 8)  & 0xff;
    pc->tx_buff[20] = crc_value & 0xff;
    pc->tx_buff[21] = PC_HEAD_2;
    usart_sendData(PC_USART, pc->tx_buff, 22);
}

void flash_ask_error(PC_DATA_t *pc)
{
    uint32_t crc_value = 0;
    pc->tx_buff[0] = PC_HEAD_1;
    pc->tx_buff[1] = 0x03;
    pc->tx_buff[2] = 0x02;
    pc->tx_buff[3] = LOCAL_DEVICE_ID_2;
    pc->tx_buff[4] = LOCAL_DEVICE_ID_1;
    pc->tx_buff[5] = 0x00;
    pc->tx_buff[6] = 0x0A;
    pc->tx_buff[7] = 0x18;
    pc->tx_buff[8] = 0x00;
    pc->tx_buff[9] = 0x00;
    pc->tx_buff[10] = 0x00;
    pc->tx_buff[11] = 0x00;
    pc->tx_buff[12] = 0x00;
    pc->tx_buff[13] = 0x00;
    pc->tx_buff[14] = 0x00;
    pc->tx_buff[15] = 0x00;
    pc->tx_buff[16] = 0x00;
    memset(pc->check_crc_buff, 0, PC_CRC_BUFF_SIZE);
    convert_data(pc->tx_buff, pc->check_crc_buff, 1, 16);
    crc_value = crc_block_calculate(pc->check_crc_buff, 4);
    crc_data_reset();
    pc->tx_buff[17] = (crc_value >> 24) & 0xff;
    pc->tx_buff[18] = (crc_value >> 16) & 0xff;
    pc->tx_buff[19] = (crc_value >> 8)  & 0xff;
    pc->tx_buff[20] = crc_value & 0xff;
    pc->tx_buff[21] = PC_HEAD_2;
    usart_sendData(PC_USART, pc->tx_buff, 22);
}
 
#define   STORAGE_INFO       0x1FFFF7E0
#define   STORAGE_INFO_SRAM
#define   STORAGE_INFO_FLASH

#define   UNIQUE_ID          0x1FFFF7E8

#define   UNIQUE_ID_0_31     0x1FFFF7E8
#define   UNIQUE_ID_32_63    0x1FFFF7EC
#define   UNIQUE_ID_64_95    0x1FFFF7F0


struct McuInfo_t
{
    uint32_t Size;
    uint8_t UniqueId[12];
} McuInfo;

uint8_t UniqueId[12];

uint32_t GetMcuInfo(void)
{
    McuInfo.Size = *(uint32_t*)STORAGE_INFO;
    uint32_t uid = 0;

    for (uint8_t i = 0; i < 12; i++)
    {
        McuInfo.UniqueId[i] = *(uint8_t*)(UNIQUE_ID + i);
    }

    memset(pc_data.check_crc_buff, 0, PC_CRC_BUFF_SIZE);
    convert_data(McuInfo.UniqueId, pc_data.check_crc_buff, 0, 12);
    uid = crc_block_calculate(pc_data.check_crc_buff, 3);
    crc_data_reset();
    return uid;
}
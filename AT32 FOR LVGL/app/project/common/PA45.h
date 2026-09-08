#ifndef __PA45_H
#define __PA45_H

#include <stdbool.h>
#include <string.h>
#include <stdbool.h>

#include "at32f403a_407.h"
#include "at32f403a_407_wk_config.h"

#include "inf_beep.h"
#include "Inf_lcd.h"
#include "Inf_touch.h"
#include "inf_w25qxxx.h"


#define SOFTWARE_VERSTION "V1.0.0"
#define HARDWARE_VERSTION "V1.0.0"

#define MAX_INPUT_AC_VOLTAGE
#define MIN_INPUT_AC_VOLTAGE

#define MAX_INPUT_DC_VOLTAGE
#define MIN_INPUT_DC_VOLTAGE

#define ADC_CH_NUM 8

typedef enum
{
    PAGE_LOGO,
    PAGE_CURVE,
    PAGE_VALUE,
    PAGE_POINTER,

    PAGE_MULTIMETER,
    PAGE_CHARGE,
    PAGE_SET,

} Page_e;

typedef enum
{
    STATE_CLOSE,
    STATE_OPEN,
} State_e;

typedef enum
{
    STATE_WAVE_PLAY,
    STATE_WAVE_PAUSE,
} WavePlayState_e;

typedef enum
{
    STATE_VOLTAGE_FLOATING_CLOSE,
    STATE_VOLTAGE_FLOATING_OPEN,
} VoltageFloatingState_e;

typedef enum
{
    STATE_WAVE_REPLAY_STOP,
    STATE_WAVE_REPLAY_PAUSE,
    STATE_WAVE_REPLAY_PLAY,
} WaveRePlayState_e;

typedef enum
{
    STATE_WAVE_RECORD_FINISH,
    STATE_WAVE_RECORD_START,

} WaveRecordState_e;

typedef enum
{
    POINTER_PAGE_SHOW_LEVEL_0,
    POINTER_PAGE_SHOW_LEVEL_1,
    POINTER_PAGE_SHOW_LEVEL_2,
    POINTER_PAGE_SHOW_LEVEL_3,
    POINTER_PAGE_SHOW_LEVEL_4,
} PointerShowLevel_e;

typedef enum
{
    INA226_SHOW_STYLE_0, // curve style
    INA226_SHOW_STYLE_1, // value style
    INA226_SHOW_STYLE_2, // pointer style
} Ina226DataShowStyle_e;

typedef enum
{
    SELECT_BEEP_OR_DIO,
    SELECT_DC,
    SELECT_AC,
    SELECT_R,
    SELECT_C,

} MultimeterCurrentSetting_e;

// Language setting
typedef enum
{
    LANG_CN, // Chinese
    LANG_EN, // English
} Language_t;

typedef struct
{
    State_e speaker;     // Sound enable (on/off)
    Language_t language; // Language setting

} Function_t;

// USB-A port measurement values
typedef struct
{
    float vbus; // VBUS voltage (V)
    float ibus; // Output current (A)
    float dp;   // D+ voltage (V)
    float dm;   // D- voltage (V)
} USB_Port_t;

// Type-C port measurement values
typedef struct
{
    float vbus; // VBUS voltage (V)
    float ibus; // Output current (A)
    float dp;   // DP (D+) voltage (V) (note: if used as USB2.0)
    float dm;   // DM (D-) voltage (V)
    float cc1;  // CC1 voltage (V)
    float cc2;  // CC2 voltage (V)
} TypeC_Port_t;

/*================== Global Variable Structure ==================*/

typedef struct
{
    Function_t function;

    Page_e page;
    Page_e last_page;

    // ========   Internal Bus  ========
    float input_ina226_voltage; // ina226 input voltage (V)
    float input_ina226_current; // ina226 input current (A)

    // ======== Port Measurement Data ========
    USB_Port_t usb;
    TypeC_Port_t typec;

    float input_hct6801_dc;
    float input_hct6801_ac;

    float input_hct6801_r;
    float input_hct6801_dio;
    float input_ne555_c;

    /* curve page */
    VoltageFloatingState_e Voltage_floating_state;

    WavePlayState_e wave_play_state;

    WavePlayState_e multimeter_wave_play_state;

    WaveRePlayState_e wave_replay_state;

    WaveRecordState_e wave_record_state;

    /* value page*/

    /* pointer page */
    PointerShowLevel_e pointer_show_level;

    /* multimeter page */
    MultimeterCurrentSetting_e multimeter_current_setting;
    MultimeterCurrentSetting_e last_multimeter_current_setting;
    Ina226DataShowStyle_e ina226_data_show_style;

    bool init_flag;

    bool touch_setting_flag;

    bool refreshing_page;
    // ======== Reset Flags (latched) ========
    bool reset_flag;
    // ======== Last Error Code ========

    /* ADC raw buffer mapping:
       adc_raw_buff[0]: USB_VOLTAGE_ADC
       adc_raw_buff[1]: USB_DP_ADC
       adc_raw_buff[2]: USB_DM_ADC
       adc_raw_buff[3]: USB_CURRENT_ADC
       adc_raw_buff[4]: TYPEC_VOLTAGE_ADC
       adc_raw_buff[5]: TYPEC_DM_ADC
       adc_raw_buff[6]: TYPEC_CC2_ADC
       adc_raw_buff[7]: TYPEC_CURRENT_ADC
       adc_raw_buff[8]: VOLTAGE_OUT_ADC
       adc_raw_buff[9]: CURRENT_OUT_ADC
       adc_raw_buff[10]: NEC_ADC
       adc_raw_buff[11]: TYPEC_DM_DP_ADC
       adc_raw_buff[12]: TYPEC_CC1_ADC
    */
    uint16_t adc_raw_buff[ADC_CH_NUM];

    bool flash_updata;

} PA45_Global_t;

/*================== Global Instance Declaration ==================*/
extern PA45_Global_t spa45;

// Initialize the global power data structure
void Power_Global_Init(void);

#endif // __PA45_H
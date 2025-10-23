#include "customDAC.h"

/*

//Codigo espressif esp32 para DAC
//https://github.com/pycom/pycom-esp-idf/blob/master/components/driver/rtc_module.c
//esp_err_t dac_output_enable(dac_channel_t channel)
//{
//    RTC_MODULE_CHECK((channel >= DAC_CHANNEL_1) && (channel < DAC_CHANNEL_MAX), DAC_ERR_STR_CHANNEL_ERROR, ESP_ERR_INVALID_ARG);
//    dac_rtc_pad_init(channel);
//    portENTER_CRITICAL(&rtc_spinlock);
//    if (channel == DAC_CHANNEL_1) {
//        SET_PERI_REG_MASK(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_XPD_DAC | RTC_IO_PDAC1_DAC_XPD_FORCE);
//    } else if (channel == DAC_CHANNEL_2) {
//        SET_PERI_REG_MASK(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_XPD_DAC | RTC_IO_PDAC2_DAC_XPD_FORCE);
//    }
//    portEXIT_CRITICAL(&rtc_spinlock);
//
//    return ESP_OK;
//}

//esp_err_t dac_output_voltage(dac_channel_t channel, uint8_t dac_value)
//{
//    RTC_MODULE_CHECK((channel >= DAC_CHANNEL_1) && (channel < DAC_CHANNEL_MAX), DAC_ERR_STR_CHANNEL_ERROR, ESP_ERR_INVALID_ARG);
//    portENTER_CRITICAL(&rtc_spinlock);
//    //Disable Tone
//    CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);
//
//    //Disable Channel Tone
//    if (channel == DAC_CHANNEL_1) {
//        CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
//    } else if (channel == DAC_CHANNEL_2) {
//        CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);
//    }
//
//    //Set the Dac value
//    if (channel == DAC_CHANNEL_1) {
//        SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, dac_value, RTC_IO_PDAC1_DAC_S);   //dac_output
//    } else if (channel == DAC_CHANNEL_2) {
//        SET_PERI_REG_BITS(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, dac_value, RTC_IO_PDAC2_DAC_S);   //dac_output
//    }
//
//    portEXIT_CRITICAL(&rtc_spinlock);
//
//    return ESP_OK;
//}

//static esp_err_t dac_rtc_pad_init(dac_channel_t channel)
//{
//    RTC_MODULE_CHECK((channel >= DAC_CHANNEL_1) && (channel < DAC_CHANNEL_MAX), DAC_ERR_STR_CHANNEL_ERROR, ESP_ERR_INVALID_ARG);
//    gpio_num_t gpio_num = 0;
//    dac_pad_get_io_num(channel, &gpio_num);
//    rtc_gpio_init(gpio_num);
//    rtc_gpio_output_disable(gpio_num);
//    rtc_gpio_input_disable(gpio_num);
//    rtc_gpio_pullup_dis(gpio_num);
//    rtc_gpio_pulldown_dis(gpio_num);
//
//    return ESP_OK;
//}

//esp_err_t rtc_gpio_init(gpio_num_t gpio_num)
//{
//    RTC_MODULE_CHECK(rtc_gpio_is_valid_gpio(gpio_num), "RTC_GPIO number error", ESP_ERR_INVALID_ARG);
//    portENTER_CRITICAL(&rtc_spinlock);
//    // 0: GPIO connected to digital GPIO module. 1: GPIO connected to analog RTC module.
//    SET_PERI_REG_MASK(rtc_gpio_desc[gpio_num].reg, (rtc_gpio_desc[gpio_num].mux));
//    //0:RTC FUNCIOTN 1,2,3:Reserved
//    SET_PERI_REG_BITS(rtc_gpio_desc[gpio_num].reg, RTC_IO_TOUCH_PAD1_FUN_SEL_V, 0x0, rtc_gpio_desc[gpio_num].func);
//    portEXIT_CRITICAL(&rtc_spinlock);
//
//    return ESP_OK;
//}


void custom_rtc_gpio_output_disable(unsigned char gpio_num)
{
 //int rtc_gpio_num = rtc_gpio_desc[gpio_num].rtc_num;
 //RTC_MODULE_CHECK(rtc_gpio_num != -1, "RTC_GPIO number error", ESP_ERR_INVALID_ARG);
 CLEAR_PERI_REG_MASK(RTC_GPIO_ENABLE_W1TS_REG, (1 << (rtc_gpio_num + RTC_GPIO_ENABLE_W1TS_S)));
 SET_PERI_REG_MASK(RTC_GPIO_ENABLE_W1TC_REG, (1 << ( rtc_gpio_num + RTC_GPIO_ENABLE_W1TC_S))); 
}


void custom_dac_rtc_pad_init(unsigned char channel)
{    
 unsigned char gpio_num = 0;
 custom_dac_pad_get_io_num(channel, &gpio_num);
 custom_rtc_gpio_init(gpio_num);
 custom_rtc_gpio_output_disable(gpio_num);
 custom_rtc_gpio_input_disable(gpio_num);
 custom_rtc_gpio_pullup_dis(gpio_num);
 custom_rtc_gpio_pulldown_dis(gpio_num);
}

void custom_rtc_gpio_input_disable(unsigned char gpio_num)
{
 //RTC_MODULE_CHECK(rtc_gpio_is_valid_gpio(gpio_num), "RTC_GPIO number error", ESP_ERR_INVALID_ARG);
 //portENTER_CRITICAL(&rtc_spinlock);
 CLEAR_PERI_REG_MASK(rtc_gpio_desc[gpio_num].reg, rtc_gpio_desc[gpio_num].ie);
 //portEXIT_CRITICAL(&rtc_spinlock);
 //return ESP_OK;
}


void custom_rtc_gpio_pulldown_dis(unsigned char gpio_num)
{
 //this is a digital pad
 if (rtc_gpio_desc[gpio_num].pulldown == 0) {
  //return ESP_ERR_INVALID_ARG;
  return;
 }

 //this is a rtc pad
 //portENTER_CRITICAL(&rtc_spinlock);
 CLEAR_PERI_REG_MASK(rtc_gpio_desc[gpio_num].reg, rtc_gpio_desc[gpio_num].pulldown);
 //portEXIT_CRITICAL(&rtc_spinlock);
 //return ESP_OK;
}

void custom_rtc_gpio_pullup_dis(unsigned char gpio_num)
{
 //this is a digital pad
 if ( rtc_gpio_desc[gpio_num].pullup == 0 ) {
  //return ESP_ERR_INVALID_ARG;
  return;
 }

 //this is a rtc pad
 //portENTER_CRITICAL(&rtc_spinlock);
 CLEAR_PERI_REG_MASK(rtc_gpio_desc[gpio_num].reg, rtc_gpio_desc[gpio_num].pullup);
 //portEXIT_CRITICAL(&rtc_spinlock);

 //return ESP_OK;
}


void custom_rtc_gpio_init(unsigned char gpio_num)
{
 //RTC_MODULE_CHECK(rtc_gpio_is_valid_gpio(gpio_num), "RTC_GPIO number error", ESP_ERR_INVALID_ARG);
 //portENTER_CRITICAL(&rtc_spinlock);
 // 0: GPIO connected to digital GPIO module. 1: GPIO connected to analog RTC module.
 SET_PERI_REG_MASK(rtc_gpio_desc[gpio_num].reg, (rtc_gpio_desc[gpio_num].mux));
 //0:RTC FUNCIOTN 1,2,3:Reserved
 SET_PERI_REG_BITS(rtc_gpio_desc[gpio_num].reg, RTC_IO_TOUCH_PAD1_FUN_SEL_V, 0x0, rtc_gpio_desc[gpio_num].func);
 //portEXIT_CRITICAL(&rtc_spinlock);
 //return ESP_OK;
}


*/



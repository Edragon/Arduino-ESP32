#include "taskADC.h"
#include "taskLogger.h"

void initADC() {
  // ADC config (ESP32): 12-bit default, set attenuation for up to ~3.3V
  #ifdef ARDUINO_ARCH_ESP32
    analogReadResolution(12);
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
    LOG_INFO_ADC("Battery ADC initialized on pin %d", BATTERY_ADC_PIN);
  #endif
}

void taskADC(void* pv) {
  (void)pv;
  LOG_INFO_ADC("Task started");
  
  for (;;) {
    // Read battery voltage (mV at ADC pin)
    uint32_t adc_mv = 0;
    #ifdef ARDUINO_ARCH_ESP32
      adc_mv = analogReadMilliVolts(BATTERY_ADC_PIN);
    #else
      int raw = analogRead(BATTERY_ADC_PIN);
      // Fallback conversion if milliVolts API not available (assuming 3.3V ref, 12-bit)
      adc_mv = (uint32_t)((raw * 3300UL) / 4095UL);
    #endif

    uint32_t batt_mv = (uint32_t)(adc_mv * BATTERY_DIVIDER_RATIO);

    if (xSemaphoreTake(controlMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
      control.battery_mv = (uint16_t)min(batt_mv, (uint32_t)65535);
      xSemaphoreGive(controlMutex);
      
      // Debug output
      LOG_DEBUG_ADC("Raw ADC: %d mV | Battery: %d mV", adc_mv, batt_mv);
    }

    vTaskDelay(pdMS_TO_TICKS(5000)); // Read every 5 seconds
  }
}

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
  AQI_ALERT_NONE = 0,
  AQI_ALERT_TEMP_HIGH,
  AQI_ALERT_TEMP_LOW,
  AQI_ALERT_HUMDIITY_HIGH,
  AQI_ALERT_HUMDIITY_LOW,
  AQI_ALERT_CO2_HIGH,
  AQI_ALERT_NOX_HIGH,
  AQI_ALERT_VOC_HIGH,
  AQI_ALERT_COUNT
} aqi_alert_reason_t;

typedef struct {
  float temp_celsius;
  bool temp_available;
  float humidity;
  bool humidity_available;
  uint16_t co2_ppm;
  bool co2_available;
  int32_t nox_index;
  bool nox_available;
  int32_t voc_index;
  bool voc_available;
} aqi_data_t;

aqi_alert_reason_t aqi_alerts_check(aqi_data_t *aqi_data);

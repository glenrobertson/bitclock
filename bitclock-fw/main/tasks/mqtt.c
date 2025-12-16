#include "mqtt.h"

#include "esp_log.h"
#include "libs/nvs.h"
#include "scd4x.h"
#include "sgp4x.h"
#include "sht4x.h"
#include "tasks.h"
#include "tasks/wifi.h"
#include <string.h>

#ifdef MQTT_TASK_ENABLED

#define TASK_DEBUGGING_ENABLED

static const char *TAG = "mqtt-task";

StaticTask_t mqttTaskBuffer;
StackType_t mqttTaskStack[MQTT_STACK_SIZE];

EventGroupHandle_t mqtt_event_group_handle;
static StaticEventGroup_t mqtt_event_group;

void get_aqi_data(aqi_data_t *aqi_data) {
  aqi_data->temp_celsius = sht4x_current_temp_celsius();
  aqi_data->humidity = sht4x_current_relative_humidity();
  aqi_data->co2_ppm = scd4x_current_co2_ppm();
  aqi_data->nox_index = sgp41_current_nox_index();
  aqi_data->voc_index = sgp4x_current_voc_index();
}

void vPrintTaskWatermarks(void);

void mqtt_task_run(void *pvParameters) {
  mqtt_event_group_handle = xEventGroupCreateStatic(&mqtt_event_group);
  TickType_t delay_time = portMAX_DELAY;

  while (1) {
    xEventGroupWaitBits(mqtt_event_group_handle,
                        MQTT_EVENT_WIFI_CONNECTED | MQTT_EVENT_URL_CHANGED,
                        true,  // clear = true
                        false, // wait for all = false
                        delay_time);

#ifdef TASK_DEBUGGING_ENABLED
    vPrintTaskWatermarks();
#endif

    const char *mqtt_url = bitclock_nvs_get_mqtt_url();

    if (mqtt_url == NULL || strlen(mqtt_url) < 1) {
      ESP_LOGI(TAG, "No mqtt_url so ignoring");
      delay_time = portMAX_DELAY;
      continue;
    } else {
      ESP_LOGI(TAG, "MQTT URL: %s", mqtt_url);
    }

    if (!bitclock_wifi_is_started()) {
      ESP_LOGI(TAG, "WiFi not started");
      delay_time = portMAX_DELAY;
      continue;
    }

    ESP_LOGI(TAG, "MQTT task running");

    aqi_data_t aqi_data = {0};
    get_aqi_data(&aqi_data);
    send_homeassistant_mqtt_sensor_data(mqtt_url, &aqi_data);
    delay_time = pdMS_TO_TICKS(60000);
  }
}

#ifdef TASK_DEBUGGING_ENABLED
void vPrintTaskWatermarks(void) {
  // Always print stack watermarks (doesn't require uxTaskGetSystemState)
  UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();

  // Allocate array to store task information (only used if trace facility is enabled)
  TaskStatus_t *pxTaskStatusArray =
      (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

  if (pxTaskStatusArray == NULL) {
    ESP_LOGI(TAG, "pvPortMalloc failed; can't print task watermarks");
  } else {

#if (configUSE_TRACE_FACILITY == 1)
    uint32_t ulTotalRunTime = 0;

    // Populate task status array
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

    ESP_LOGI(TAG, "Task Name\t\tWatermark");
    ESP_LOGI(TAG, "------------------------------------");

    for (UBaseType_t x = 0; x < uxArraySize; x++) {
      UBaseType_t wm = uxTaskGetStackHighWaterMark(pxTaskStatusArray[x].xHandle);
      ESP_LOGI(TAG, "%-20s\t%lu words", pxTaskStatusArray[x].pcTaskName,
               (unsigned long)wm);
    }
#else
    // Trace facility disabled: we can't enumerate task handles via uxTaskGetSystemState
    ESP_LOGI(TAG, "configUSE_TRACE_FACILITY=0; enable it to list all tasks. "
                  "You can still log individual tasks if you keep their TaskHandle_t.");
#endif

    vPortFree(pxTaskStatusArray);
  }

  // Min heap remaining
  size_t min_free_heap = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
  ESP_LOGI(TAG, "Min free heap: %u", (unsigned)min_free_heap);
}
#endif

#endif

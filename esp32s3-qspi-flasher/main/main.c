#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "flash.h"

static const char *TAG = "main";

void app_main(void)
{
    flash_quad_cfg_t cfg = {
        .host = SPI2_HOST,
        .cs = GPIO_NUM_10,
        .clk = GPIO_NUM_12,
        .io0 = GPIO_NUM_11,
        .io1 = GPIO_NUM_13,
        .io2 = GPIO_NUM_14,
        .io3 = GPIO_NUM_9,
        .clock_hz = 20 * 1000 * 1000,
    };

    ESP_ERROR_CHECK(flash_init(&cfg));

    flash_jedec_id_t id = {0};
    ESP_ERROR_CHECK(flash_read_jedec_id(&id));

    flash_status_t st = {0};
    ESP_ERROR_CHECK(flash_read_status(&st));

    ESP_LOGI(TAG, "JEDEC=%02X %02X %02X",
             id.manufacturer_id, id.memory_type, id.capacity_id);

    ESP_LOGI(TAG, "Status1=0x%02X Status2=0x%02X Status3=0x%02X",
             st.status1, st.status2, st.status3);

    flash_probe_result_t probe = FLASH_PROBE_UNKNOWN;
    ESP_ERROR_CHECK(flash_probe_write_path(&probe));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
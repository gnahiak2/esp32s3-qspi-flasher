#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK  12
#define PIN_NUM_CS   10
#define PIN_NUM_WP   14
#define PIN_NUM_HD   9

spi_device_handle_t spi;

void setup_pullups(void)
{
    gpio_set_pull_mode(PIN_NUM_CS, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_NUM_WP, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_NUM_HD, GPIO_PULLUP_ONLY);
}

esp_err_t spi_init(void)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = PIN_NUM_WP,
        .quadhd_io_num = PIN_NUM_HD,
        .max_transfer_sz = 4096,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000, // 1 MHz
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(
        SPI2_HOST,
        &buscfg,
        SPI_DMA_CH_AUTO
    ));

    return spi_bus_add_device(
        SPI2_HOST,
        &devcfg,
        &spi
    );
}

void read_jedec_id(void)
{
    uint8_t cmd = 0x9F;
    uint8_t id[3] = {0};

    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };

    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));

    memset(&t, 0, sizeof(t));

    t.length = 24;
    t.rxlength = 24;
    t.rx_buffer = id;

    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));

    printf("JEDEC ID: %02X %02X %02X\n",
           id[0], id[1], id[2]);
}

void app_main(void)
{
    setup_pullups();

    ESP_ERROR_CHECK(spi_init());

    vTaskDelay(pdMS_TO_TICKS(100));

    while (1) {
        read_jedec_id();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
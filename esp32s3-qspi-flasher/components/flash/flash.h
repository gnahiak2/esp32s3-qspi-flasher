#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

typedef struct {
    spi_host_device_t host;
    gpio_num_t cs;
    gpio_num_t clk;
    gpio_num_t io0; // MOSI / DI
    gpio_num_t io1; // MISO / DO
    gpio_num_t io2; // WP#
    gpio_num_t io3; // HOLD#/RESET#
    uint32_t clock_hz;
} flash_quad_cfg_t;

typedef struct {
    uint8_t manufacturer_id;
    uint8_t memory_type;
    uint8_t capacity_id;
    uint8_t jedec_id[3];
} flash_jedec_id_t;

typedef struct {
    uint8_t status1;
    uint8_t status2;
    uint8_t status3;
    bool has_status2;
    bool has_status3;
} flash_status_t;

typedef enum {
    FLASH_PROBE_UNKNOWN = 0,
    FLASH_PROBE_OK,
    FLASH_PROBE_WRITE_ENABLE_FAIL,
    FLASH_PROBE_WRITE_PROTECT_SUSPECTED,
} flash_probe_result_t;

esp_err_t flash_init(const flash_quad_cfg_t *cfg);
esp_err_t flash_read_jedec_id(flash_jedec_id_t *out);
esp_err_t flash_read_status(flash_status_t *out);
esp_err_t flash_write_enable(void);
esp_err_t flash_write_disable(void);
esp_err_t flash_read(uint32_t addr, void *buf, size_t len);
esp_err_t flash_probe_write_path(flash_probe_result_t *result);
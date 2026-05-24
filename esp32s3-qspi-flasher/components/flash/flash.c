#include "flash.h"

#include <string.h>
#include "esp_log.h"

static const char *TAG = "flash";
static spi_device_handle_t s_dev = NULL;

static esp_err_t flash_tx(const void *txbuf, int bits)
{
    spi_transaction_t t = {
        .length = bits,
        .tx_buffer = txbuf,
    };
    return spi_device_transmit(s_dev, &t);
}

static esp_err_t flash_txrx(const void *txbuf, void *rxbuf, int bits)
{
    spi_transaction_t t = {
        .length = bits,
        .tx_buffer = txbuf,
        .rx_buffer = rxbuf,
    };
    return spi_device_transmit(s_dev, &t);
}

esp_err_t flash_init(const flash_quad_cfg_t *cfg)
{
    if (!cfg) {
        return ESP_ERR_INVALID_ARG;
    }

    spi_bus_config_t buscfg = {
        .mosi_io_num = cfg->io0,
        .miso_io_num = cfg->io1,
        .sclk_io_num = cfg->clk,
        .quadwp_io_num = cfg->io2,
        .quadhd_io_num = cfg->io3,
        .max_transfer_sz = 4096,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = cfg->clock_hz,
        .mode = 0,
        .spics_io_num = cfg->cs,
        .queue_size = 1,
    };

    esp_err_t err = spi_bus_initialize(cfg->host, &buscfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
        return err;
    }

    err = spi_bus_add_device(cfg->host, &devcfg, &s_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "SPI bus initialized");
    return ESP_OK;
}

esp_err_t flash_read_jedec_id(flash_jedec_id_t *out)
{
    if (!out || !s_dev) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t tx[] = {0x9F, 0x00, 0x00, 0x00};
    uint8_t rx[4] = {0};

    spi_transaction_t t = {
        .length = 32,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };

    esp_err_t err = spi_device_transmit(s_dev, &t);
    if (err != ESP_OK) {
        return err;
    }

    out->jedec_id[0] = rx[1];
    out->jedec_id[1] = rx[2];
    out->jedec_id[2] = rx[3];
    out->manufacturer_id = rx[1];
    out->memory_type = rx[2];
    out->capacity_id = rx[3];

    ESP_LOGI(TAG, "JEDEC ID: %02X %02X %02X",
             out->jedec_id[0], out->jedec_id[1], out->jedec_id[2]);
    return ESP_OK;
}

esp_err_t flash_read_status(flash_status_t *out)
{
    if (!out || !s_dev) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));

    uint8_t tx1[] = {0x05, 0x00};
    uint8_t rx1[] = {0, 0};
    spi_transaction_t t1 = {
        .length = 16,
        .tx_buffer = tx1,
        .rx_buffer = rx1,
    };
    esp_err_t err = spi_device_transmit(s_dev, &t1);
    if (err != ESP_OK) {
        return err;
    }
    out->status1 = rx1[1];

    uint8_t tx2[] = {0x35, 0x00};
    uint8_t rx2[] = {0, 0};
    spi_transaction_t t2 = {
        .length = 16,
        .tx_buffer = tx2,
        .rx_buffer = rx2,
    };
    err = spi_device_transmit(s_dev, &t2);
    if (err == ESP_OK) {
        out->status2 = rx2[1];
        out->has_status2 = true;
    }

    uint8_t tx3[] = {0x15, 0x00};
    uint8_t rx3[] = {0, 0};
    spi_transaction_t t3 = {
        .length = 16,
        .tx_buffer = tx3,
        .rx_buffer = rx3,
    };
    err = spi_device_transmit(s_dev, &t3);
    if (err == ESP_OK) {
        out->status3 = rx3[1];
        out->has_status3 = true;
    }

    ESP_LOGI(TAG, "Status1=0x%02X Status2=0x%02X Status3=0x%02X",
             out->status1, out->status2, out->status3);
    return ESP_OK;
}

esp_err_t flash_write_enable(void)
{
    if (!s_dev) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t cmd = 0x06;
    return flash_tx(&cmd, 8);
}

esp_err_t flash_write_disable(void)
{
    if (!s_dev) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t cmd = 0x04;
    return flash_tx(&cmd, 8);
}

esp_err_t flash_read(uint32_t addr, void *buf, size_t len)
{
    if (!buf || !len || !s_dev) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t cmd[4] = {
        0x03,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)addr,
    };

    esp_err_t err = flash_tx(cmd, 32);
    if (err != ESP_OK) {
        return err;
    }

    spi_transaction_t t = {
        .length = len * 8,
        .rx_buffer = buf,
    };
    return spi_device_transmit(s_dev, &t);
}

esp_err_t flash_probe_write_path(flash_probe_result_t *result)
{
    if (!result) {
        return ESP_ERR_INVALID_ARG;
    }

    *result = FLASH_PROBE_UNKNOWN;

    flash_status_t before = {0};
    esp_err_t err = flash_read_status(&before);
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "Before WREN: SR1=0x%02X SR2=0x%02X SR3=0x%02X",
             before.status1, before.status2, before.status3);

    err = flash_write_enable();
    if (err != ESP_OK) {
        *result = FLASH_PROBE_WRITE_ENABLE_FAIL;
        return err;
    }

    flash_status_t after = {0};
    err = flash_read_status(&after);
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "After WREN: SR1=0x%02X SR2=0x%02X SR3=0x%02X",
             after.status1, after.status2, after.status3);

    if ((after.status1 & 0x02) == 0) {
        ESP_LOGW(TAG, "WEL bit did not set");
        *result = FLASH_PROBE_WRITE_ENABLE_FAIL;
        return ESP_OK;
    }

    if ((after.status1 & 0x1C) != 0 || (after.status2 & 0x02) != 0 || (after.status3 != 0)) {
        ESP_LOGW(TAG, "Protection bits may be active");
        *result = FLASH_PROBE_WRITE_PROTECT_SUSPECTED;
        return ESP_OK;
    }

    *result = FLASH_PROBE_OK;
    ESP_LOGI(TAG, "Write path appears OK");
    return ESP_OK;
}
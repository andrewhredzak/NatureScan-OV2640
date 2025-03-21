#include "esp_camera.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include "OV2640.h"
#include "OV2640.c"
#include "esp_log.h"



void app_main(void)
{
printf("      ✴       ✴ <<<MADMANINDUSTRIES>>> ✴     ✴           \n");


// Initialize camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        printf("Camera init failed: 0x%x\n", err);
        return;
    }

    // Initialize SPI bus for SD card
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    err = spi_bus_initialize(VSPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        printf("SPI init failed: 0x%x\n", err);
        return;
    }

    // Mount SD card
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = VSPI_HOST;

    err = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (err != ESP_OK) {
        printf("SD mount failed: 0x%x\n", err);
        return;
    }
    printf("SD mount successful\n");

    // Create capture task
    xTaskCreatePinnedToCore(capture_task, "capture_task", 4096, NULL, 5, NULL, 1);


}

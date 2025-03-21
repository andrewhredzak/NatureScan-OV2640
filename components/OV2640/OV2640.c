#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "OV2640.h"


//static const char *TAG = "OV2640.c";  //tag for source code


#define MOUNT_POINT "/sdcard"

// SD card pins (ESP32-CAM specific)
#define PIN_NUM_MISO 2   // SD MISO
#define PIN_NUM_MOSI 15  // SD MOSI
#define PIN_NUM_CLK  14  // SD SCK
#define PIN_NUM_CS   13  // SD CS

// Camera configuration for OV2640 on ESP32-CAM
static camera_config_t camera_config = {
    .pin_pwdn = 32,
    .pin_reset = -1,  // No reset pin on ESP32-CAM
    .pin_xclk = 0,
    .pin_sscb_sda = 26,  // SCCB SDA
    .pin_sscb_scl = 27,  // SCCB SCL
    .pin_d7 = 35,
    .pin_d6 = 34,
    .pin_d5 = 39,
    .pin_d4 = 36,
    .pin_d3 = 21,
    .pin_d2 = 19,
    .pin_d1 = 18,
    .pin_d0 = 5,
    .pin_vsync = 25,
    .pin_href = 23,
    .pin_pclk = 22,
    .xclk_freq_hz = 20000000,  // 20 MHz XCLK
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_RGB565,  // Uncompressed, 16-bit
    .frame_size = FRAMESIZE_UXGA,      // 1632x1232
    .jpeg_quality = 10,  // Unused for RGB565
    .fb_count = 1        // Single frame buffer
};

// BMP header (24-bit, UXGA, bottom-up)
static const uint8_t bmp_header[54] = {
    0x42, 0x4D,                    // 'BM'
    0x36, 0x0A, 0x5C, 0x00,        // File size: 6,031,926 (54 + 1632*1232*3)
    0x00, 0x00, 0x00, 0x00,        // Reserved
    0x36, 0x00, 0x00, 0x00,        // Offset to pixel data: 54
    0x28, 0x00, 0x00, 0x00,        // Info header size: 40
    0x60, 0x06, 0x00, 0x00,        // Width: 1632
    0xD0, 0x04, 0x00, 0x00,        // Height: 1232
    0x01, 0x00,                    // Planes: 1
    0x18, 0x00,                    // Bits per pixel: 24
    0x00, 0x00, 0x00, 0x00,        // Compression: 0 (none)
    0x00, 0x0A, 0x5C, 0x00,        // Image size: 6,031,872
    0x00, 0x00, 0x00, 0x00,        // X ppm: 0
    0x00, 0x00, 0x00, 0x00,        // Y ppm: 0
    0x00, 0x00, 0x00, 0x00,        // Colors used: 0
    0x00, 0x00, 0x00, 0x00         // Important colors: 0
};

// Capture and save task
static void capture_task(void *pvParameters) {
    static int image_counter = 0;
    char filename[32];

    while (1) {
        // Generate unique filename
        sprintf(filename, MOUNT_POINT"/image_%04d.bmp", image_counter++);

        // Capture frame
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            printf("Capture failed\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        // Open file
        FILE *f = fopen(filename, "wb");
        if (!f) {
            printf("File open failed: %s\n", filename);
            esp_camera_fb_return(fb);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        // Write BMP header
        fwrite(bmp_header, 1, 54, f);

        // Convert and write pixel data (bottom-up)
        for (int i = fb->height - 1; i >= 0; i--) {
            uint8_t *row_start = fb->buf + i * fb->width * 2;
            uint8_t row_buf[1632 * 3];  // 4896 bytes for one UXGA row
            for (int j = 0; j < fb->width; j++) {
                uint16_t pixel = (row_start[j * 2] << 8) | row_start[j * 2 + 1];
                uint8_t r5 = (pixel >> 11) & 0x1F;
                uint8_t g6 = (pixel >> 5) & 0x3F;
                uint8_t b5 = pixel & 0x1F;
                row_buf[j * 3] = (b5 << 3) | (b5 >> 2);      // B8
                row_buf[j * 3 + 1] = (g6 << 2) | (g6 >> 4);  // G8
                row_buf[j * 3 + 2] = (r5 << 3) | (r5 >> 2);  // R8
            }
            fwrite(row_buf, 1, fb->width * 3, f);
        }

        fclose(f);
        esp_camera_fb_return(fb);
        printf("Saved: %s\n", filename);

        // Wait 3 seconds
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}
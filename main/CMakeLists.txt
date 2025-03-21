idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES OV2640 
                    REQUIRES esp_camera fatfs sdmmc spi_flash)

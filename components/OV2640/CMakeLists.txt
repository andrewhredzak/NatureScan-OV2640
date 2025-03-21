idf_component_register(SRCS "OV2640.c"
                    REQUIRES esp_camera fatfs sdmmc spi_flash 
                    INCLUDE_DIRS ".")
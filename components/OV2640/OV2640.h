#ifndef OV2640_H
#define OV2640_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"




//OV2640 prototypes
static void capture_task(void *pvParameters);




#ifdef __cplusplus
}
#endif
#endif /* OV2640_H */
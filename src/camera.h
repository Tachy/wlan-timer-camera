#pragma once

#include "esp_camera.h"
#include "esp_err.h"

esp_err_t      camera_init(void);
void           camera_deinit(void);
esp_err_t      camera_capture(camera_fb_t **fb);
void           camera_fb_return(camera_fb_t *fb);

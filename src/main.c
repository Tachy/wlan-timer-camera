#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "nvs_flash.h"

#include "config.h"
#include "camera.h"
#include "wifi_conn.h"
#include "scp_upload.h"

static const char *TAG = "main";

static void capture_cycle(void)
{
    camera_fb_t *fb = NULL;
    if (camera_capture(&fb) != ESP_OK) {
        ESP_LOGE(TAG, "Bildaufnahme fehlgeschlagen");
        return;
    }

    int rssi_pct = 0;
    if (wifi_connect(&rssi_pct) != ESP_OK) {
        ESP_LOGE(TAG, "WLAN fehlgeschlagen — Bild wird verworfen");
        camera_fb_return(fb);
        return;
    }

    if (scp_upload(fb->buf, fb->len, IMAGE_FILENAME) != ESP_OK)
        ESP_LOGE(TAG, "SCP-Upload fehlgeschlagen");

    if (ssh_exec_append_log(rssi_pct) != ESP_OK)
        ESP_LOGW(TAG, "Signal-Log fehlgeschlagen");

    camera_fb_return(fb);
    wifi_disconnect();
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== WLAN Timer-Kamera gestartet ===");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    if (camera_init() != ESP_OK) {
        ESP_LOGE(TAG, "Kamera-Init fehlgeschlagen — Neustart nötig");
        return;
    }

    if (wifi_init() != ESP_OK) {
        ESP_LOGE(TAG, "WLAN-Init fehlgeschlagen — Neustart nötig");
        return;
    }

    for (;;) {
        capture_cycle();
        gpio_hold_en(PIN_FLASH_LED);
        esp_sleep_enable_timer_wakeup((uint64_t)CAPTURE_INTERVAL_MS * 1000ULL);
        esp_light_sleep_start();
        gpio_hold_dis(PIN_FLASH_LED);
    }
}

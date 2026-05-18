#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "nvs_flash.h"

#include "config.h"
#include "camera.h"
#include "wifi_conn.h"
#include "scp_upload.h"

static const char *TAG = "main";

// ── Hilfsfunktionen ──────────────────────────────────────────────────────────

static void signal_done(void)
{
    gpio_config_t io = {
        .pin_bit_mask  = (1ULL << PIN_DONE),
        .mode          = GPIO_MODE_OUTPUT,
        .pull_up_en    = GPIO_PULLUP_DISABLE,
        .pull_down_en  = GPIO_PULLDOWN_DISABLE,
        .intr_type     = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(PIN_DONE, 1);
    // 200 ms halten, damit TPL5110 den Impuls sicher erkennt
    vTaskDelay(pdMS_TO_TICKS(200));
}

// ── Hauptprogramm ────────────────────────────────────────────────────────────

void app_main(void)
{
    ESP_LOGI(TAG, "=== WLAN Timer-Kamera gestartet ===");

    // NVS initialisieren (wird vom WiFi-Subsystem benötigt)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // 1. Kamera initialisieren
    if (camera_init() != ESP_OK) {
        ESP_LOGE(TAG, "Kamera-Init fehlgeschlagen — System wird abgeschaltet");
        signal_done();
        return;
    }

    // 2. Bild aufnehmen
    camera_fb_t *fb = NULL;
    if (camera_capture(&fb) != ESP_OK) {
        ESP_LOGE(TAG, "Bildaufnahme fehlgeschlagen");
        signal_done();
        return;
    }

    // 3. WLAN verbinden
    if (wifi_connect() != ESP_OK) {
        ESP_LOGE(TAG, "WLAN fehlgeschlagen — Bild wird verworfen");
        camera_fb_return(fb);
        signal_done();
        return;
    }

    // 4. Bild per SCP hochladen
    esp_err_t up = scp_upload(fb->buf, fb->len, IMAGE_FILENAME);
    if (up != ESP_OK) {
        ESP_LOGE(TAG, "SCP-Upload fehlgeschlagen");
    }

    // 5. Aufräumen
    camera_fb_return(fb);
    wifi_disconnect();

    // 6. DONE-Signal → TPL5110 trennt die Stromversorgung
    ESP_LOGI(TAG, "Sende DONE auf GPIO %d", PIN_DONE);
    signal_done();

    // Sollte nicht erreicht werden — TPL5110 schaltet ab
    ESP_LOGW(TAG, "Warte auf Stromabschaltung durch TPL5110 …");
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

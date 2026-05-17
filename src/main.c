#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "nvs.h"

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

// Liest den Boot-Zähler aus NVS und inkrementiert ihn.
// Gibt den *alten* Wert zurück (für den Dateinamen).
static uint32_t get_and_increment_counter(void)
{
    nvs_handle_t h;
    uint32_t counter = 0;

    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK) {
        nvs_get_u32(h, NVS_KEY_COUNTER, &counter);
        nvs_set_u32(h, NVS_KEY_COUNTER, counter + 1);
        nvs_commit(h);
        nvs_close(h);
    }
    return counter;
}

static void make_filename(char *buf, size_t buflen, uint32_t counter)
{
    snprintf(buf, buflen, "img_%06" PRIu32 ".jpg", counter);
}

// ── Hauptprogramm ────────────────────────────────────────────────────────────

void app_main(void)
{
    ESP_LOGI(TAG, "=== WLAN Timer-Kamera gestartet ===");

    // NVS initialisieren (wird auch von WiFi-Subsystem benötigt)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    uint32_t counter = get_and_increment_counter();
    char filename[32];
    make_filename(filename, sizeof(filename), counter);
    ESP_LOGI(TAG, "Dateiname: %s", filename);

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
    esp_err_t up = scp_upload(fb->buf, fb->len, filename);
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

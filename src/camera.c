#include "camera.h"
#include "config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "camera";

// Pin-Belegung ESP32-CAM AI-Thinker
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK     0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

static void flash_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << PIN_FLASH_LED),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(PIN_FLASH_LED, 0);
}

esp_err_t camera_init(void)
{
    flash_init();

    camera_config_t cfg = {
        .pin_pwdn     = CAM_PIN_PWDN,
        .pin_reset    = CAM_PIN_RESET,
        .pin_xclk     = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7       = CAM_PIN_D7,
        .pin_d6       = CAM_PIN_D6,
        .pin_d5       = CAM_PIN_D5,
        .pin_d4       = CAM_PIN_D4,
        .pin_d3       = CAM_PIN_D3,
        .pin_d2       = CAM_PIN_D2,
        .pin_d1       = CAM_PIN_D1,
        .pin_d0       = CAM_PIN_D0,
        .pin_vsync    = CAM_PIN_VSYNC,
        .pin_href     = CAM_PIN_HREF,
        .pin_pclk     = CAM_PIN_PCLK,

        .xclk_freq_hz = 20000000,
        .ledc_timer   = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG,
        .frame_size   = CAM_FRAME_SIZE,
        .jpeg_quality = CAM_JPEG_QUALITY,
        .fb_count     = 1,
        .fb_location  = CAMERA_FB_IN_PSRAM,
        .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
    };

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_camera_init fehlgeschlagen: %s", esp_err_to_name(err));
    }
    return err;
}

void camera_deinit(void)
{
    esp_camera_deinit();
}

esp_err_t camera_capture(camera_fb_t **fb)
{
    // LED einschalten und warten, bis der Sensor mindestens einen
    // vollständig belichteten Frame mit Blitzlicht aufgenommen hat.
    // Bei UXGA und 20 MHz XCLK beträgt die Framerate ~5–8 fps → 200 ms
    // entsprechen 1–2 Frames Vorlauf, was für eine stabile Belichtung reicht.
    gpio_set_level(PIN_FLASH_LED, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    *fb = esp_camera_fb_get();

    gpio_set_level(PIN_FLASH_LED, 0);

    if (!*fb) {
        ESP_LOGE(TAG, "Bildaufnahme fehlgeschlagen");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Bild aufgenommen: %zu Bytes, %ux%u px", (*fb)->len, (*fb)->width, (*fb)->height);
    return ESP_OK;
}

void camera_fb_return(camera_fb_t *fb)
{
    esp_camera_fb_return(fb);
}

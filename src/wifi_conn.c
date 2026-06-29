#include "wifi_conn.h"
#include "config.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "wifi";

#define CONNECTED_BIT BIT0
#define FAIL_BIT      BIT1

static EventGroupHandle_t s_events;

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Verbindung getrennt");
        xEventGroupSetBits(s_events, FAIL_BIT);
    }
}

static void on_ip_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        xEventGroupSetBits(s_events, CONNECTED_BIT);
    }
}

esp_err_t wifi_connect(int *rssi_pct_out)
{
    *rssi_pct_out = 0;
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    s_events = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    esp_event_handler_instance_t h_wifi, h_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi_event, NULL, &h_wifi));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, on_ip_event, NULL, &h_ip));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(
        s_events, CONNECTED_BIT | FAIL_BIT,
        pdFALSE, pdFALSE,
        pdMS_TO_TICKS(WIFI_TIMEOUT_MS));

    if (bits & CONNECTED_BIT) {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            int pct = (ap_info.rssi + 100) * 2;
            if (pct < 0)   pct = 0;
            if (pct > 100) pct = 100;
            *rssi_pct_out = pct;
            ESP_LOGI(TAG, "WLAN-Signal: %d dBm (%d %%)", ap_info.rssi, pct);
        }
        return ESP_OK;
    }

    ESP_LOGE(TAG, "WLAN-Verbindung fehlgeschlagen");
    return ESP_FAIL;
}

void wifi_disconnect(void)
{
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_event_loop_delete_default();
    esp_netif_deinit();
}

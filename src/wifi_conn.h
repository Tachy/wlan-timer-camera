#pragma once

#include "esp_err.h"

// Einmalige Initialisierung — muss vor dem ersten wifi_connect() aufgerufen werden.
esp_err_t wifi_init(void);

// Verbindet mit WLAN. Schreibt die Signalstärke in Prozent (0–100) nach *rssi_pct_out.
esp_err_t wifi_connect(int *rssi_pct_out);
void      wifi_disconnect(void);

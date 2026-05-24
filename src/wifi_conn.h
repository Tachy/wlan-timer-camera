#pragma once

#include "esp_err.h"

// Verbindet mit WLAN. Schreibt die Signalstärke in Prozent (0–100) nach *rssi_pct_out.
// Bei Verbindungsfehler wird *rssi_pct_out auf 0 gesetzt.
esp_err_t wifi_connect(int *rssi_pct_out);
void      wifi_disconnect(void);

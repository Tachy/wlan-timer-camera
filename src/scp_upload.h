#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

// Lädt `data` (len Bytes) via SCP auf den konfigurierten Server hoch.
// remote_filename: Dateiname relativ zu SCP_REMOTE_DIR (z. B. "img_000042.jpg")
esp_err_t scp_upload(const uint8_t *data, size_t len, const char *remote_filename);

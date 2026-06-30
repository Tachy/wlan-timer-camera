#include "scp_upload.h"
#include "config.h"
#include "ssh_key_data.h"
#include "esp_log.h"

#include <libssh2.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "scp";

static int tcp_connect(void)
{
    struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM };
    struct addrinfo *res = NULL;
    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", SCP_PORT);

    if (getaddrinfo(SCP_HOST, port_str, &hints, &res) != 0 || !res) {
        ESP_LOGE(TAG, "DNS-Auflösung für %s fehlgeschlagen", SCP_HOST);
        return -1;
    }

    int sock = socket(res->ai_family, res->ai_socktype, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket() fehlgeschlagen");
        freeaddrinfo(res);
        return -1;
    }

    struct timeval tv = { .tv_sec = SCP_TIMEOUT_SEC };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE(TAG, "TCP connect zu %s:%d fehlgeschlagen", SCP_HOST, SCP_PORT);
        close(sock);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    ESP_LOGI(TAG, "TCP verbunden mit %s:%d", SCP_HOST, SCP_PORT);
    return sock;
}

esp_err_t scp_upload(const uint8_t *data, size_t len, const char *remote_filename)
{
    int sock = -1;
    LIBSSH2_SESSION *session = NULL;
    LIBSSH2_CHANNEL *channel = NULL;
    esp_err_t result = ESP_FAIL;

    sock = tcp_connect();
    if (sock < 0) return ESP_FAIL;

    if (libssh2_init(0) != 0) {
        ESP_LOGE(TAG, "libssh2_init fehlgeschlagen");
        goto cleanup;
    }

    session = libssh2_session_init();
    if (!session) {
        ESP_LOGE(TAG, "libssh2_session_init fehlgeschlagen");
        goto cleanup;
    }

    libssh2_session_set_blocking(session, 1);

    if (libssh2_session_handshake(session, sock) != 0) {
        char *msg;
        libssh2_session_last_error(session, &msg, NULL, 0);
        ESP_LOGE(TAG, "SSH-Handshake fehlgeschlagen: %s", msg);
        goto cleanup;
    }

    int auth_rc = libssh2_userauth_publickey_frommemory(
        session,
        SCP_USER, strlen(SCP_USER),
        NULL, 0,                                    // public key: libssh2 leitet ihn ab
        ssh_privkey_data, ssh_privkey_len,
        SSH_KEY_PASSPHRASE
    );
    if (auth_rc != 0) {
        char *msg;
        libssh2_session_last_error(session, &msg, NULL, 0);
        ESP_LOGE(TAG, "SSH-Authentifizierung fehlgeschlagen: %s", msg);
        goto cleanup;
    }

    char remote_path[256];
    snprintf(remote_path, sizeof(remote_path), "%s%s", SCP_REMOTE_DIR, remote_filename);

    channel = libssh2_scp_send64(session, remote_path, 0644,
                                  (libssh2_int64_t)len, 0, 0);
    if (!channel) {
        char *msg;
        libssh2_session_last_error(session, &msg, NULL, 0);
        ESP_LOGE(TAG, "SCP-Kanal fehlgeschlagen: %s", msg);
        goto cleanup;
    }

    size_t sent = 0;
    while (sent < len) {
        ssize_t n = libssh2_channel_write(channel,
                                           (const char *)data + sent,
                                           len - sent);
        if (n < 0) {
            ESP_LOGE(TAG, "Schreibfehler: %d", (int)n);
            goto cleanup;
        }
        sent += (size_t)n;
    }

    libssh2_channel_send_eof(channel);

    ESP_LOGI(TAG, "Upload abgeschlossen: %s (%zu Bytes)", remote_path, len);
    result = ESP_OK;

cleanup:
    if (channel) libssh2_channel_free(channel);
    if (session) {
        libssh2_session_disconnect(session, "Normal shutdown");
        libssh2_session_free(session);
    }
    libssh2_exit();
    if (sock >= 0) close(sock);
    return result;
}

esp_err_t ssh_exec_append_log(int rssi_pct)
{
    int sock = -1;
    LIBSSH2_SESSION *session = NULL;
    LIBSSH2_CHANNEL *channel = NULL;
    esp_err_t result = ESP_FAIL;

    sock = tcp_connect();
    if (sock < 0) return ESP_FAIL;

    if (libssh2_init(0) != 0) {
        ESP_LOGE(TAG, "libssh2_init fehlgeschlagen (log)");
        goto log_cleanup;
    }

    session = libssh2_session_init();
    if (!session) goto log_cleanup;

    libssh2_session_set_blocking(session, 1);

    if (libssh2_session_handshake(session, sock) != 0) goto log_cleanup;

    if (libssh2_userauth_publickey_frommemory(
            session,
            SCP_USER, strlen(SCP_USER),
            NULL, 0,
            ssh_privkey_data, ssh_privkey_len,
            SSH_KEY_PASSPHRASE) != 0) {
        ESP_LOGE(TAG, "SSH-Auth fehlgeschlagen (log)");
        goto log_cleanup;
    }

    channel = libssh2_channel_open_session(session);
    if (!channel) {
        ESP_LOGE(TAG, "SSH-Channel fehlgeschlagen (log)");
        goto log_cleanup;
    }

    char cmd[320];
    snprintf(cmd, sizeof(cmd),
        "echo \"$(date '+%%Y-%%m-%%d %%H:%%M:%%S %%Z')  WLAN-Signal: %d %%\" "
        ">> " SCP_REMOTE_DIR LOG_FILENAME,
        rssi_pct);

    if (libssh2_channel_exec(channel, cmd) != 0) {
        ESP_LOGE(TAG, "SSH exec fehlgeschlagen");
        goto log_cleanup;
    }

    libssh2_channel_send_eof(channel);

    ESP_LOGI(TAG, "Signal-Log geschrieben: %d %%", rssi_pct);
    result = ESP_OK;

log_cleanup:
    if (channel) libssh2_channel_free(channel);
    if (session) {
        libssh2_session_disconnect(session, "Normal shutdown");
        libssh2_session_free(session);
    }
    libssh2_exit();
    if (sock >= 0) close(sock);
    return result;
}

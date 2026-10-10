/* Remote node: reads a sensor on UART, sends MAVLink over UDP/Wi-Fi.
 *   - HEARTBEAT at 1 Hz (link-loss detection on the base station)
 *   - NAMED_VALUE_FLOAT "SENSOR" per reading, time_boot_ms = sample time on THIS device
 * Needs third_party/mavlink (run tools/fetch_mavlink.sh).
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "mavlink.h"
#include "sensor.h"

#define TAG "remote"
#define SYS_ID  42
#define COMP_ID MAV_COMP_ID_ONBOARD_COMPUTER

static EventGroupHandle_t s_ev;
#define WIFI_UP BIT0
static int s_sock = -1;
static struct sockaddr_in s_dest;

static void wifi_evt(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) esp_wifi_connect();
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_ev, WIFI_UP);
        ESP_LOGW(TAG, "wifi lost, reconnecting");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_ev, WIFI_UP);
    }
}

static void wifi_start(void)
{
    s_ev = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_evt, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_evt, NULL));
    wifi_config_t wc = { 0 };
    strncpy((char *)wc.sta.ssid, CONFIG_RN_WIFI_SSID, sizeof wc.sta.ssid - 1);
    strncpy((char *)wc.sta.password, CONFIG_RN_WIFI_PASS, sizeof wc.sta.password - 1);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void send_msg(const mavlink_message_t *msg)
{
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t n = mavlink_msg_to_send_buffer(buf, msg);
    sendto(s_sock, buf, n, 0, (struct sockaddr *)&s_dest, sizeof s_dest);
}

static uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

static void heartbeat_task(void *arg)
{
    for (;;) {
        xEventGroupWaitBits(s_ev, WIFI_UP, pdFALSE, pdTRUE, portMAX_DELAY);
        mavlink_message_t m;
        mavlink_msg_heartbeat_pack(SYS_ID, COMP_ID, &m, MAV_TYPE_GENERIC,
                                   MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_ACTIVE);
        send_msg(&m);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void sensor_task(void *arg)
{
    float v;
    for (;;) {
        if (!sensor_read(&v, 1000)) { ESP_LOGW(TAG, "no sensor data"); continue; }
        uint32_t t = now_ms();                 /* timestamp at read time, before any network delay */
        if (!(xEventGroupGetBits(s_ev) & WIFI_UP)) continue;
        mavlink_message_t m;
        mavlink_msg_named_value_float_pack(SYS_ID, COMP_ID, &m, t, "SENSOR", v);
        send_msg(&m);
    }
}

void app_main(void)
{
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    sensor_init();
    wifi_start();

    s_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    memset(&s_dest, 0, sizeof s_dest);
    s_dest.sin_family = AF_INET;
    s_dest.sin_port = htons(CONFIG_RN_DEST_PORT);
    inet_pton(AF_INET, CONFIG_RN_DEST_IP, &s_dest.sin_addr);

    xTaskCreate(heartbeat_task, "hb", 4096, NULL, 4, NULL);
    xTaskCreate(sensor_task, "sensor", 4096, NULL, 5, NULL);
}

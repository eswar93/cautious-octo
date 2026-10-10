#include "sensor.h"
#include <stdlib.h>
#include <string.h>
#include "driver/uart.h"
#include "sdkconfig.h"

#define RX_BUF 512
static char line[64];
static size_t line_len;

void sensor_init(void)
{
    const uart_config_t cfg = {
        .baud_rate = CONFIG_RN_SENSOR_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_RN_SENSOR_UART_NUM, RX_BUF * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_RN_SENSOR_UART_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(CONFIG_RN_SENSOR_UART_NUM, CONFIG_RN_SENSOR_TX_PIN,
                                 CONFIG_RN_SENSOR_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static bool sensor_parse_line(const char *s, float *out)
{
    char *end;
    float v = strtof(s, &end);
    if (end == s) return false;
    *out = v;
    return true;
}

bool sensor_read(float *value, int timeout_ms)
{
    uint8_t c;
    while (uart_read_bytes(CONFIG_RN_SENSOR_UART_NUM, &c, 1, pdMS_TO_TICKS(timeout_ms)) == 1) {
        if (c == '\n' || c == '\r') {
            if (line_len > 0) {
                line[line_len] = 0;
                line_len = 0;
                if (sensor_parse_line(line, value)) return true;
            }
        } else if (line_len < sizeof line - 1) {
            line[line_len++] = (char)c;
        } else {
            line_len = 0;  /* overlong garbage line: resync */
        }
    }
    return false;
}

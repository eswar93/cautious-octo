#pragma once
#include <stdbool.h>
void sensor_init(void);
/* Blocks up to timeout_ms for one reading. Returns false on timeout.
 * Assumes the sensor emits ASCII lines containing one float, e.g. "23.41\r\n".
 * CHANGE sensor_parse_line() in sensor.c to match your real sensor's format. */
bool sensor_read(float *value, int timeout_ms);

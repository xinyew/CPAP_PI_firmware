#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic interface for sensors.
 */
typedef struct {
    const char *name;
    int (*init)(void);
    int (*read)(void);
    uint32_t (*get_interval_ms)(void);
    uint32_t last_read_time_ms;
} sensor_interface_t;

int sensor_manager_init(void);
void sensor_manager_poll(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_MANAGER_H */

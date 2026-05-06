#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t ppg_red;
    uint32_t ppg_ir;
    uint32_t ppg_green;
    int32_t  force_raw;
    int32_t  force_mv;      /* TIA Output Voltage */
    int32_t  force_vref_mv; /* Measured Vref */
    float    force_res_ohm; /* Calculated FSR Resistance */
    float    temp_c;
    float    humidity_rh;
} system_sensor_data_t;

extern system_sensor_data_t current_sensor_data;

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
void sensor_manager_report(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_MANAGER_H */

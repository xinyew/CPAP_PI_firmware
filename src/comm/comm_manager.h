#ifndef COMM_MANAGER_H
#define COMM_MANAGER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic interface for communication methods.
 */
typedef enum {
    COMM_IF_SERIAL = 0,
    COMM_IF_BLE,
    COMM_IF_COUNT
} comm_interface_type_t;

typedef struct {
    const char *name;
    int (*init)(void);
    int (*send_data)(const char *data, size_t len);
} comm_interface_t;

int comm_manager_init(void);
void comm_manager_broadcast(const char *data, size_t len);
void comm_manager_set_interface(comm_interface_type_t type);
comm_interface_type_t comm_manager_get_interface(void);

#ifdef __cplusplus
}
#endif

#endif /* COMM_MANAGER_H */

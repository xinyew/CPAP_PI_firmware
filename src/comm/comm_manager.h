#ifndef COMM_MANAGER_H
#define COMM_MANAGER_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Generic interface for communication methods.
 */
typedef struct {
    const char *name;
    int (*init)(void);
    int (*send_data)(const char *data, size_t len);
} comm_interface_t;

int comm_manager_init(void);
void comm_manager_broadcast(const char *data, size_t len);

#endif /* COMM_MANAGER_H */

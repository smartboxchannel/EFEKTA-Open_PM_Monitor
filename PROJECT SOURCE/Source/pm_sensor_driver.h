#ifndef _SENSOR_DRIVER_H_
#define _SENSOR_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>

/*********************************************************************
 * TYPEDEFS
 *********************************************************************/

// Sensor type enumeration
typedef enum {
    SENSOR_TYPE_UNKNOWN = 0,
    SENSOR_TYPE_APM10,
    SENSOR_TYPE_APM2000
} sensor_type_t;

/*********************************************************************
 * FUNCTION PROTOTYPES
 *********************************************************************/

// Initialize sensor with auto-detection
void sensor_init(void);

// Read data from sensor
bool sensor_read(uint16_t* pm1, uint16_t* pm2_5, uint16_t* pm10);

// Get sensor type for debugging
sensor_type_t sensor_get_type(void);

// Legacy functions for backward compatibility
void apm_start(void);
void apm_stop(void);
bool apm_read(uint16_t* pm1, uint16_t* pm2_5, uint16_t* pm10);

#endif /* _SENSOR_DRIVER_H_ */
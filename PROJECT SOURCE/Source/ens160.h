/*********************************************************************
 * ENS160 Driver - SCIO Sensors Air Quality Sensor
 *********************************************************************/

#ifndef ENS160_H
#define ENS160_H

#include <stdint.h>
#include <stdbool.h>

/*********************************************************************
 * I2C Addresses (8-bit)
 *********************************************************************/

#define ENS160_ADDR_READ          (0x53 << 1) | 0x01  // 0xA7
#define ENS160_ADDR_WRITE         (0x53 << 1) | 0x00  // 0xA6

/*********************************************************************
 * Register Map
 *********************************************************************/

#define ENS160_REG_PART_ID        0x00
#define ENS160_REG_REV_ID         0x01
#define ENS160_REG_OPMODE         0x10
#define ENS160_REG_CONFIG         0x11
#define ENS160_REG_COMMAND        0x12
#define ENS160_REG_TEMP_IN        0x13
#define ENS160_REG_RH_IN          0x15
#define ENS160_REG_DATA_STATUS    0x20
#define ENS160_REG_DATA_AQI       0x21
#define ENS160_REG_DATA_TVOC      0x22
#define ENS160_REG_DATA_ECO2      0x24

/*********************************************************************
 * Constants
 *********************************************************************/

#define ENS160_PART_ID_VAL        0x0160

// Commands
#define ENS160_CMD_NOP            0x00
#define ENS160_CMD_RESET          0x11
#define ENS160_CMD_HEATER_OFF     0x20
#define ENS160_CMD_HEATER_ON      0x21

// Operation modes
#define ENS160_MODE_DEEP_SLEEP    0x00
#define ENS160_MODE_IDLE          0x01
#define ENS160_MODE_STANDARD      0x02

// Status bits (datasheet page 28)
#define ENS160_STATUS_NEWDAT      0x02   // New data available in DATA_x registers
#define ENS160_STATUS_NEWGPR      0x01   // New data in GPR_READ register
#define ENS160_STATUS_VALIDITY_MASK 0x0C // Validity bits (2-3)
#define ENS160_STATUS_VALIDITY_SHIFT 2

/*********************************************************************
 * Return Codes
 *********************************************************************/

#define ENS160_ERROR               1
#define ENS160_SUCCESS             0

/*********************************************************************
 * Data Structure
 *********************************************************************/

typedef struct {
    uint16_t aqi;       // Air Quality Index (1-5)
    uint16_t tvoc;      // TVOC (ppb)
    uint16_t eco2;      // eCO2 (ppm)
    uint8_t validity;   // 0=normal, 1=warm-up, 2=initializing, 3=invalid
    bool data_ready;    // New data available flag
} ens160_data_t;

/*********************************************************************
 * Function Prototypes
 *********************************************************************/

// Initialization
int8_t ens160_init(void);
int8_t ens160_get_version(uint16_t *part_id, uint8_t *rev_id);
int8_t ens160_reset(void);
int8_t ens160_set_mode(uint8_t mode);
int8_t ens160_get_mode(uint8_t *mode);

// Heater control
int8_t ens160_heater_enable(void);
int8_t ens160_heater_disable(void);

// Environmental compensation
void ens160_set_compensation(float temperature, float humidity);

// Status
int8_t ens160_get_status(uint8_t *status);
int8_t ens160_is_data_ready(bool *ready);
uint8_t ens160_get_validity(void);

// Data reading
int8_t ens160_read_aqi(uint16_t *aqi, uint8_t *validity);
int8_t ens160_read_tvoc(uint16_t *tvoc);
int8_t ens160_read_eco2(uint16_t *eco2);
int8_t ens160_read_data(ens160_data_t *data);

// Utilities
const char* ens160_validity_to_string(uint8_t validity);
void ens160_WaitUs(uint16_t microSecs);
void ens160_WaitMs(uint32_t period);

#endif // ENS160_H
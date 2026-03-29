/*********************************************************************
 * INCLUDES
 *********************************************************************/

#include "pm_sensor_driver.h"
#include "hal_i2c.h"
#include <stdlib.h>

/*********************************************************************
 * CONSTANTS
 *********************************************************************/

// I2C addresses
#define SENSOR_ADDR             0x08
#define APM_ADDR_READ           ((SENSOR_ADDR << 1) | 0x01)  // 0x11
#define APM_ADDR_WRITE          ((SENSOR_ADDR << 1) | 0x00)  // 0x10

// Command lengths
#define START_CMD_LENGTH        5
#define STOP_CMD_LENGTH         2
#define READ_CMD_LENGTH         2

// Command data
static const uint8_t start_command[START_CMD_LENGTH] = {0x00, 0x10, 0x05, 0x00, 0xF6};
static const uint8_t stop_command[STOP_CMD_LENGTH] = {0x01, 0x04};
static const uint8_t read_command[READ_CMD_LENGTH] = {0x03, 0x00};

/*********************************************************************
 * LOCAL VARIABLES
 *********************************************************************/

// Current sensor type
static sensor_type_t current_sensor = SENSOR_TYPE_UNKNOWN;

/*********************************************************************
 * LOCAL FUNCTIONS PROTOTYPES
 *********************************************************************/

static uint8_t calc_crc8(uint8_t* data, uint8_t length);
static sensor_type_t detect_sensor(void);
static bool read_apm10(uint16_t* pm1, uint16_t* pm2_5, uint16_t* pm10);
static bool read_apm2000(uint16_t* pm1, uint16_t* pm2_5, uint16_t* pm10);

// Delay function (declared in zcl_app.c)
extern void DelayMs(unsigned int delaytime);

/*********************************************************************
 * @fn      calc_crc8
 *
 * @brief   CRC8 calculation for APM2000 sensor
 *********************************************************************/

static uint8_t calc_crc8(uint8_t* data, uint8_t length)
{
    uint8_t crc = 0xFF;
    uint8_t i, bit;
    
    for (i = 0; i < length; i++) {
        crc ^= data[i];
        for (bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

/*********************************************************************
 * @fn      detect_sensor
 *
 * @brief   Detect connected sensor type (APM10 or APM2000)
 *********************************************************************/

static sensor_type_t detect_sensor(void)
{
    uint8_t test_data[30] = {0};
    int i;
    
    // Send read command
    HalI2CSend(APM_ADDR_WRITE, (uint8_t*)read_command, READ_CMD_LENGTH);
    
    // Try to read 30 bytes (maximum format)
    HalI2CReceive(APM_ADDR_READ, test_data, 30);
    
    // Check for APM2000: CRC bytes present and data non-zero
    bool has_nonzero = false;
    for (i = 0; i < 30; i++) {
        if (test_data[i] != 0) {
            has_nonzero = true;
            break;
        }
    }
    
    if (has_nonzero) {
        // Check CRC for APM2000
        uint8_t crc_buf[2];
        
        // Check PM1 CRC
        crc_buf[0] = test_data[0];
        crc_buf[1] = test_data[1];
        if (calc_crc8(crc_buf, 2) == test_data[2]) {
            return SENSOR_TYPE_APM2000;
        }
        
        // Check PM2.5 CRC
        crc_buf[0] = test_data[3];
        crc_buf[1] = test_data[4];
        if (calc_crc8(crc_buf, 2) == test_data[5]) {
            return SENSOR_TYPE_APM2000;
        }
    }
    
    // Check for APM10: first 12 bytes contain data
    uint16_t pm1 = (test_data[0] << 8) | test_data[1];
    uint16_t pm2_5 = (test_data[3] << 8) | test_data[4];
    uint16_t pm10 = (test_data[9] << 8) | test_data[10];
    
    if (pm1 != 0 || pm2_5 != 0 || pm10 != 0) {
        return SENSOR_TYPE_APM10;
    }
    
    return SENSOR_TYPE_UNKNOWN;
}

/*********************************************************************
 * @fn      read_apm10
 *
 * @brief   Read data from APM10 sensor
 *********************************************************************/

static bool read_apm10(uint16_t* pm1, uint16_t* pm2_5, uint16_t* pm10)
{
    uint8_t data[12] = {0};
    
    HalI2CSend(APM_ADDR_WRITE, (uint8_t*)read_command, READ_CMD_LENGTH);
    HalI2CReceive(APM_ADDR_READ, data, 12);
    
    *pm1 = (data[0] << 8) | data[1];
    *pm2_5 = (data[3] << 8) | data[4];
    *pm10 = (data[9] << 8) | data[10];
    
    return (*pm1 != 0xFFFF && *pm2_5 != 0xFFFF && *pm10 != 0xFFFF);
}

/*********************************************************************
 * @fn      read_apm2000
 *
 * @brief   Read data from APM2000 sensor with CRC validation
 *********************************************************************/

static bool read_apm2000(uint16_t* pm1, uint16_t* pm2_5, uint16_t* pm10)
{
    uint8_t data[30] = {0};
    uint8_t crc_buf[2];
    
    HalI2CSend(APM_ADDR_WRITE, (uint8_t*)read_command, READ_CMD_LENGTH);
    HalI2CReceive(APM_ADDR_READ, data, 30);
    
    // PM1
    crc_buf[0] = data[0];
    crc_buf[1] = data[1];
    if (calc_crc8(crc_buf, 2) == data[2]) {
        *pm1 = (data[0] << 8) | data[1];
    } else {
        *pm1 = 0xFFFF;
    }
    
    // PM2.5
    crc_buf[0] = data[3];
    crc_buf[1] = data[4];
    if (calc_crc8(crc_buf, 2) == data[5]) {
        *pm2_5 = (data[3] << 8) | data[4];
    } else {
        *pm2_5 = 0xFFFF;
    }
    
    // PM10
    crc_buf[0] = data[9];
    crc_buf[1] = data[10];
    if (calc_crc8(crc_buf, 2) == data[11]) {
        *pm10 = (data[9] << 8) | data[10];
    } else {
        *pm10 = 0xFFFF;
    }
    
    return (*pm1 != 0xFFFF && *pm2_5 != 0xFFFF && *pm10 != 0xFFFF);
}

/*********************************************************************
 * PUBLIC FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      sensor_init
 *
 * @brief   Initialize sensor with auto-detection
 *********************************************************************/

void sensor_init(void)
{
    HalI2CInit();
    DelayMs(2500);
    
    // Send start command
    HalI2CSend(APM_ADDR_WRITE, (uint8_t*)start_command, START_CMD_LENGTH);
    DelayMs(2500);
    
    // Detect sensor type
    current_sensor = detect_sensor();
    
    // Small delay after detection
    DelayMs(100);
}

/*********************************************************************
 * @fn      sensor_read
 *
 * @brief   Read data from detected sensor
 *********************************************************************/

bool sensor_read(uint16_t* pm1, uint16_t* pm2_5, uint16_t* pm10)
{
    if (current_sensor == SENSOR_TYPE_APM10) {
        return read_apm10(pm1, pm2_5, pm10);
    } else if (current_sensor == SENSOR_TYPE_APM2000) {
        return read_apm2000(pm1, pm2_5, pm10);
    }
    
    return false;
}

/*********************************************************************
 * @fn      sensor_get_type
 *
 * @brief   Get detected sensor type
 *********************************************************************/

sensor_type_t sensor_get_type(void)
{
    return current_sensor;
}

/*********************************************************************
 * LEGACY FUNCTIONS (Backward Compatibility)
 *********************************************************************/

void apm_start(void)
{
    HalI2CSend(APM_ADDR_WRITE, (uint8_t*)start_command, START_CMD_LENGTH);
}

void apm_stop(void)
{
    HalI2CSend(APM_ADDR_WRITE, (uint8_t*)stop_command, STOP_CMD_LENGTH);
}

bool apm_read(uint16_t* pm1, uint16_t* pm2_5, uint16_t* pm10)
{
    return sensor_read(pm1, pm2_5, pm10);
}
/*********************************************************************
 * INCLUDES
 *********************************************************************/

#include "ens160.h"
#include "hal_i2c.h"

/*********************************************************************
 * LOCAL FUNCTIONS PROTOTYPES
 *********************************************************************/

static uint8_t ens160_read_register(uint8_t reg, uint8_t *buffer, uint16_t len);
static uint8_t ens160_write_register(uint8_t reg, uint8_t *buffer, uint16_t len);

/*********************************************************************
 * @fn      ens160_read_register
 * @brief   Read data from ENS160 register via I2C
 * @param   reg - register address
 * @param   buffer - pointer to receive data
 * @param   len - number of bytes to read
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

static uint8_t ens160_read_register(uint8_t reg, uint8_t *buffer, uint16_t len)
{
    uint8_t txBuf[1];
    uint8_t retry = 5;
    
    if (!buffer || len == 0) {
        return ENS160_ERROR;
    }
    
    txBuf[0] = reg;
    
    while (retry--) {
        if (HalI2CSend(ENS160_ADDR_WRITE, txBuf, 1) == 0) {
            break;
        }
        ens160_WaitMs(10);
    }
    if (retry == 0 && HalI2CSend(ENS160_ADDR_WRITE, txBuf, 1) != 0) {
        return ENS160_ERROR;
    }
    
    retry = 5;
    while (retry--) {
        if (HalI2CReceive(ENS160_ADDR_READ, buffer, len) == 0) {
            return ENS160_SUCCESS;
        }
        ens160_WaitMs(10);
    }
    
    return ENS160_ERROR;
}

/*********************************************************************
 * @fn      ens160_write_register
 * @brief   Write data to ENS160 register via I2C
 * @param   reg - register address
 * @param   buffer - pointer to data to write
 * @param   len - number of bytes to write
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

static uint8_t ens160_write_register(uint8_t reg, uint8_t *buffer, uint16_t len)
{
    uint8_t txBuf[16];
    uint16_t i;
    uint8_t retry = 5;
    
    if (!buffer || len == 0) {
        return ENS160_ERROR;
    }
    
    txBuf[0] = reg;
    for (i = 0; i < len; i++) {
        txBuf[1 + i] = buffer[i];
    }
    
    while (retry--) {
        if (HalI2CSend(ENS160_ADDR_WRITE, txBuf, len + 1) == 0) {
            return ENS160_SUCCESS;
        }
        ens160_WaitMs(10);
    }
    
    return ENS160_ERROR;
}

/*********************************************************************
 * @fn      ens160_init
 * @brief   Initialize ENS160 sensor
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

int8_t ens160_init(void)
{
    uint16_t part_id = 0;
    uint8_t rev_id = 0;
    uint8_t retry = 3;
    
    // Verify PART_ID
    while (retry--) {
        if (ens160_get_version(&part_id, &rev_id) == ENS160_SUCCESS) {
            break;
        }
        ens160_WaitMs(100);
    }
    
    if (part_id != ENS160_PART_ID_VAL) {
        return ENS160_ERROR;
    }
    
    // Reset sensor
    retry = 3;
    while (retry--) {
        if (ens160_reset() == ENS160_SUCCESS) {
            break;
        }
        ens160_WaitMs(100);
    }
    
    ens160_WaitMs(50);
    
    // Set to standard operation mode
    retry = 3;
    while (retry--) {
        if (ens160_set_mode(ENS160_MODE_STANDARD) == ENS160_SUCCESS) {
            break;
        }
        ens160_WaitMs(100);
    }
    
    ens160_WaitMs(20);
    
    return ENS160_SUCCESS;
}

/*********************************************************************
 * @fn      ens160_get_version
 * @brief   Read PART_ID and REV_ID registers
 * @param   part_id - pointer to store part ID
 * @param   rev_id - pointer to store revision ID
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

int8_t ens160_get_version(uint16_t *part_id, uint8_t *rev_id)
{
    uint8_t buffer[2];
    
    if (!part_id || !rev_id) {
        return ENS160_ERROR;
    }
    
    // Read PART_ID (little-endian)
    if (ens160_read_register(ENS160_REG_PART_ID, buffer, 2) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    *part_id = buffer[0] | (buffer[1] << 8);
    
    // Read REV_ID
    if (ens160_read_register(ENS160_REG_REV_ID, rev_id, 1) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    
    return ENS160_SUCCESS;
}

/*********************************************************************
 * @fn      ens160_reset
 * @brief   Reset ENS160 sensor
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

int8_t ens160_reset(void)
{
    uint8_t cmd = ENS160_CMD_RESET;
    
    if (ens160_write_register(ENS160_REG_COMMAND, &cmd, 1) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    
    return ENS160_SUCCESS;
}

/*********************************************************************
 * @fn      ens160_set_mode
 * @brief   Set sensor operation mode
 * @param   mode - operation mode (DEEP_SLEEP, IDLE, STANDARD)
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

int8_t ens160_set_mode(uint8_t mode)
{
    if (ens160_write_register(ENS160_REG_OPMODE, &mode, 1) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    
    return ENS160_SUCCESS;
}

/*********************************************************************
 * @fn      ens160_get_mode
 * @brief   Get current operation mode
 * @param   mode - pointer to store current mode
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

int8_t ens160_get_mode(uint8_t *mode)
{
    if (!mode) {
        return ENS160_ERROR;
    }
    
    if (ens160_read_register(ENS160_REG_OPMODE, mode, 1) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    
    return ENS160_SUCCESS;
}

/*********************************************************************
 * @fn      ens160_heater_enable
 * @brief   Enable internal heater
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

int8_t ens160_heater_enable(void)
{
    uint8_t cmd = ENS160_CMD_HEATER_ON;
    
    if (ens160_write_register(ENS160_REG_COMMAND, &cmd, 1) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    
    return ENS160_SUCCESS;
}

/*********************************************************************
 * @fn      ens160_heater_disable
 * @brief   Disable internal heater
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

int8_t ens160_heater_disable(void)
{
    uint8_t cmd = ENS160_CMD_HEATER_OFF;
    
    if (ens160_write_register(ENS160_REG_COMMAND, &cmd, 1) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    
    return ENS160_SUCCESS;
}

/*********************************************************************
 * @fn      ens160_set_compensation
 * @brief   Set temperature and humidity compensation values
 * @param   temperature - temperature in Celsius
 * @param   humidity - relative humidity in percent
 *********************************************************************/

void ens160_set_compensation(float temperature, float humidity)
{
    uint8_t buffer[4];
    uint16_t temp_kelvin;
    uint16_t hum_int;
    float temp_k;
    
    // Temperature conversion: Kelvin with 1/64 resolution
    temp_k = temperature + 273.15f;
    
    if (temp_k < 233.15f) temp_k = 233.15f;
    if (temp_k > 373.15f) temp_k = 373.15f;
    
    temp_kelvin = (uint16_t)(temp_k * 64.0f);
    
    // Humidity conversion: with 1/512 resolution
    if (humidity < 0.0f) humidity = 0.0f;
    if (humidity > 100.0f) humidity = 100.0f;
    
    hum_int = (uint16_t)(humidity * 512.0f);
    
    // Write compensation data (little-endian)
    buffer[0] = temp_kelvin & 0xFF;
    buffer[1] = (temp_kelvin >> 8) & 0xFF;
    buffer[2] = hum_int & 0xFF;
    buffer[3] = (hum_int >> 8) & 0xFF;
    
    ens160_write_register(ENS160_REG_TEMP_IN, buffer, 4);
}

/*********************************************************************
 * @fn      ens160_get_status
 * @brief   Read status register
 * @param   status - pointer to store status byte
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

int8_t ens160_get_status(uint8_t *status)
{
    if (!status) {
        return ENS160_ERROR;
    }
    
    if (ens160_read_register(ENS160_REG_DATA_STATUS, status, 1) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    
    return ENS160_SUCCESS;
}

/*********************************************************************
 * @fn      ens160_is_data_ready
 * @brief   Check if new sensor data is available
 * @param   ready - pointer to store ready flag
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

int8_t ens160_is_data_ready(bool *ready)
{
    uint8_t status;
    
    if (!ready) {
        return ENS160_ERROR;
    }
    
    if (ens160_get_status(&status) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    
    *ready = (status & ENS160_STATUS_NEWDAT) != 0;
    
    return ENS160_SUCCESS;
}

/*********************************************************************
 * @fn      ens160_get_validity
 * @brief   Get data validity status
 * @return  Validity code (0-3) or 0xFF on error
 *********************************************************************/

uint8_t ens160_get_validity(void)
{
    uint8_t status;
    if (ens160_get_status(&status) == ENS160_SUCCESS) {
        return (status >> ENS160_STATUS_VALIDITY_SHIFT) & 0x03;
    }
    return 0xFF;
}

/*********************************************************************
 * @fn      ens160_read_aqi
 * @brief   Read Air Quality Index value
 * @param   aqi - pointer to store AQI value (1-5)
 * @param   validity - pointer to store validity code
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

int8_t ens160_read_aqi(uint16_t *aqi, uint8_t *validity)
{
    uint8_t buffer;
    uint8_t status;
    
    if (!aqi || !validity) {
        return ENS160_ERROR;
    }
    
    // Read AQI register (lower 3 bits contain AQI)
    if (ens160_read_register(ENS160_REG_DATA_AQI, &buffer, 1) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    *aqi = buffer & 0x07;
    
    // Read validity from status register
    if (ens160_get_status(&status) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    *validity = (status >> ENS160_STATUS_VALIDITY_SHIFT) & 0x03;
    
    return ENS160_SUCCESS;
}

/*********************************************************************
 * @fn      ens160_read_tvoc
 * @brief   Read Total Volatile Organic Compounds value
 * @param   tvoc - pointer to store TVOC in ppb
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

int8_t ens160_read_tvoc(uint16_t *tvoc)
{
    uint8_t buffer[2];
    
    if (!tvoc) {
        return ENS160_ERROR;
    }
    
    // Read TVOC (little-endian)
    if (ens160_read_register(ENS160_REG_DATA_TVOC, buffer, 2) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    *tvoc = buffer[0] | (buffer[1] << 8);
    
    return ENS160_SUCCESS;
}

/*********************************************************************
 * @fn      ens160_read_eco2
 * @brief   Read equivalent CO2 value
 * @param   eco2 - pointer to store eCO2 in ppm
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

int8_t ens160_read_eco2(uint16_t *eco2)
{
    uint8_t buffer[2];
    
    if (!eco2) {
        return ENS160_ERROR;
    }
    
    // Read eCO2 (little-endian)
    if (ens160_read_register(ENS160_REG_DATA_ECO2, buffer, 2) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    *eco2 = buffer[0] | (buffer[1] << 8);
    
    return ENS160_SUCCESS;
}

/*********************************************************************
 * @fn      ens160_read_data
 * @brief   Read all sensor data at once
 * @param   data - pointer to data structure to fill
 * @return  ENS160_SUCCESS or ENS160_ERROR
 *********************************************************************/

int8_t ens160_read_data(ens160_data_t *data)
{
    uint8_t aqi_byte;
    uint8_t tvoc_buf[2];
    uint8_t eco2_buf[2];
    uint8_t status;
    
    if (!data) {
        return ENS160_ERROR;
    }
    
    // Read AQI
    if (ens160_read_register(ENS160_REG_DATA_AQI, &aqi_byte, 1) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    
    // Read TVOC
    if (ens160_read_register(ENS160_REG_DATA_TVOC, tvoc_buf, 2) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    
    // Read eCO2
    if (ens160_read_register(ENS160_REG_DATA_ECO2, eco2_buf, 2) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    
    // Read status
    if (ens160_get_status(&status) != ENS160_SUCCESS) {
        return ENS160_ERROR;
    }
    
    // Populate structure
    data->aqi = aqi_byte & 0x07;
    data->tvoc = tvoc_buf[0] | (tvoc_buf[1] << 8);
    data->eco2 = eco2_buf[0] | (eco2_buf[1] << 8);
    data->validity = (status >> ENS160_STATUS_VALIDITY_SHIFT) & 0x03;
    data->data_ready = (status & ENS160_STATUS_NEWDAT) != 0;
    
    return ENS160_SUCCESS;
}

/*********************************************************************
 * @fn      ens160_validity_to_string
 * @brief   Convert validity code to human-readable string
 * @param   validity - validity code (0-3)
 * @return  Pointer to string description
 *********************************************************************/

const char* ens160_validity_to_string(uint8_t validity)
{
    switch (validity) {
        case 0: return "Normal operation";
        case 1: return "Warm-up phase";
        case 2: return "Initial Start-up phase";
        case 3: return "Invalid output";
        default: return "Unknown";
    }
}

/*********************************************************************
 * @fn      ens160_WaitUs
 * @brief   Microsecond delay using NOP loops
 * @param   microSecs - number of microseconds to wait
 *********************************************************************/

void ens160_WaitUs(uint16_t microSecs)
{
    while (microSecs--) {
        asm("nop"); asm("nop"); asm("nop"); asm("nop");
        asm("nop"); asm("nop"); asm("nop"); asm("nop");
        asm("nop"); asm("nop"); asm("nop"); asm("nop");
        asm("nop"); asm("nop"); asm("nop"); asm("nop");
        asm("nop"); asm("nop"); asm("nop"); asm("nop");
        asm("nop"); asm("nop"); asm("nop"); asm("nop");
        asm("nop"); asm("nop"); asm("nop"); asm("nop");
        asm("nop"); asm("nop"); asm("nop"); asm("nop");
    }
}

/*********************************************************************
 * @fn      ens160_WaitMs
 * @brief   Millisecond delay
 * @param   period - number of milliseconds to wait
 *********************************************************************/

void ens160_WaitMs(uint32_t period)
{
    while (period--) {
        ens160_WaitUs(1000);
    }
}
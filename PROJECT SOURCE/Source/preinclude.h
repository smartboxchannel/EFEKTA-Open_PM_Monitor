/* ------------------------------------------------------------------------------------------------
 * Pre-include configuration file for Zigbee device
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 * Network Security Settings
 * ------------------------------------------------------------------------------------------------
 */
#define TC_LINKKEY_JOIN
#define NV_INIT
#define NV_RESTORE

#define TP2_LEGACY_ZC
// Patch SDK
// #define ZDSECMGR_TC_ATTEMPT_DEFAULT_KEY TRUE

/* ------------------------------------------------------------------------------------------------
 * Network Layer Settings
 * ------------------------------------------------------------------------------------------------
 */
#define NWK_AUTO_POLL
#define MULTICAST_ENABLED FALSE

/* ------------------------------------------------------------------------------------------------
 * ZCL Cluster Support
 * ------------------------------------------------------------------------------------------------
 */
#define ZCL_READ
#define ZCL_WRITE
#define ZCL_BASIC
#define ZCL_IDENTIFY
#define ZCL_ON_OFF
#define ZCL_LEVEL_CTRL
#define ZCL_REPORTING_DEVICE

/* ------------------------------------------------------------------------------------------------
 * OTA (Over-The-Air) Update Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define OTA_CLIENT TRUE
#define OTA_HA
#define OTA_MANUFACTURER_ID                           0x5678
#define OTA_TYPE_ID                                   0x1000

/* ------------------------------------------------------------------------------------------------
 * Device Type Selection
 * ------------------------------------------------------------------------------------------------
 */
// #define ZSTACK_DEVICE_BUILD (DEVICE_BUILD_ENDDEVICE)
#define ZSTACK_DEVICE_BUILD (DEVICE_BUILD_ROUTER)

/* ------------------------------------------------------------------------------------------------
 * Base Device Behavior (BDB) Settings
 * ------------------------------------------------------------------------------------------------
 */
#define DISABLE_GREENPOWER_BASIC_PROXY
#define BDB_FINDING_BINDING_CAPABILITY_ENABLED 1
#define BDB_REPORTING TRUE
#define BDB_MAX_CLUSTERENDPOINTS_REPORTING 10

/* ------------------------------------------------------------------------------------------------
 * HAL (Hardware Abstraction Layer) Configuration
 * ------------------------------------------------------------------------------------------------
 */
// #define ISR_KEYINTERRUPT
#define HAL_BUZZER FALSE
#define HAL_I2C TRUE
// #define HAL_LED TRUE
// #define BLINK_LEDS TRUE

#define INT_HEAP_LEN (2500-0x146)

/* ------------------------------------------------------------------------------------------------
 * Debug Configuration
 * ------------------------------------------------------------------------------------------------
 */
// #define DO_DEBUG_UART
// #define HAL_UART TRUE
// #define HAL_UART_DMA 1  // uart0
// #define HAL_UART_ISR 2

/* ------------------------------------------------------------------------------------------------
 * Button Configuration for Factory Reset and Commissioning
 * ------------------------------------------------------------------------------------------------
 */
#define FACTORY_RESET_BY_LONG_PRESS_PORT 0x04 // P2 port for factory reset
#define APP_COMMISSIONING_BY_LONG_PRESS TRUE
#define APP_COMMISSIONING_BY_LONG_PRESS_PORT 0x04 // P2 port for commissioning
#define HAL_KEY_P2_INPUT_PINS BV(0) // P2.0 as key input

/* ------------------------------------------------------------------------------------------------
 * I2C Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define OCM_CLK_PORT 1
#define OCM_DATA_PORT 1
#define OCM_CLK_PIN 6
#define OCM_DATA_PIN 7
#define HAL_I2C_RETRY_CNT 2

/* ------------------------------------------------------------------------------------------------
 * RF Power Amplifier Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define OUTDOOR_LONG_RANGE

#ifdef OUTDOOR_LONG_RANGE
#define HAL_PA_LNA_CC2592
#define APP_TX_POWER TX_PWR_PLUS_19
#endif

/* ------------------------------------------------------------------------------------------------
 * System Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_board_cfg.h"

#include "stdint.h"
#include "stddef.h"
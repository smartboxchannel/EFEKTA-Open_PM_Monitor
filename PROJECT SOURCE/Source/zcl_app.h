/*********************************************************************
 * ZCL_APP.H - ZCL Application Header
 *********************************************************************/

#ifndef ZCL_APP_H
#define ZCL_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 *********************************************************************/

#include "zcl.h"

/*********************************************************************
 * CONSTANTS - Application Events
 *********************************************************************/

#define APP_REPORT_EVT                  0x0001
#define APP_READ_SENSORS_EVT            0x0002
#define APP_SAVE_ATTRS_EVT              0x0004
#define APP_SEND_SENSORS_EVT            0x0008
#define APP_LED_EVT                     0x0010
#define APP_IDENT_EVT                   0x0020

// Application report delay (10 seconds for testing)
#define APP_REPORT_DELAY                ((uint32) 10000)

/*********************************************************************
 * CONSTANTS - NV Memory
 *********************************************************************/

#define NW_APP_CONFIG                   0x0401

/*********************************************************************
 * MACROS - ZCL Attribute Access Control
 *********************************************************************/

#define R                               ACCESS_CONTROL_READ
#define RR                              (R | ACCESS_REPORTABLE)
#define RW                              (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_WRITE)
#define RRW                             (ACCESS_CONTROL_READ | ACCESS_REPORTABLE | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_WRITE)

/*********************************************************************
 * CONSTANTS - ZCL Clusters
 *********************************************************************/

#define BASIC                           ZCL_CLUSTER_ID_GEN_BASIC
#define ZCL_PM25                        0x042A
#define ZCL_PM1                         0x042C
#define ZCL_PM10                        0x042D
#define IDENTIFY                        ZCL_CLUSTER_ID_GEN_IDENTIFY
#define ANALOG_INPUT                    ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC

/*********************************************************************
 * CONSTANTS - ZCL Data Types (aliases for convenience)
 *********************************************************************/

#define ZCL_BOOLEAN                     ZCL_DATATYPE_BOOLEAN
#define ZCL_UINT8                       ZCL_DATATYPE_UINT8
#define ZCL_UINT16                      ZCL_DATATYPE_UINT16
#define ZCL_UINT32                      ZCL_DATATYPE_UINT32
#define ZCL_INT16                       ZCL_DATATYPE_INT16
#define ZCL_INT8                        ZCL_DATATYPE_INT8
#define ZCL_BITMAP8                     ZCL_DATATYPE_BITMAP8
#define ZCL_ENUM8                       ZCL_DATATYPE_ENUM8
#define ZCL_UNKNOWN                     ZCL_DATATYPE_UNKNOWN
#define ZCL_OCTET_STR                   ZCL_DATATYPE_OCTET_STR
#define ZCL_CHAR_STR                    ZCL_DATATYPE_CHAR_STR
#define ZCL_ARRAY                       ZCL_DATATYPE_ARRAY
#define ZCL_SINGLE                      ZCL_DATATYPE_SINGLE_PREC

/*********************************************************************
 * CONSTANTS - PM Sensor Attribute IDs
 *********************************************************************/

#define ATTRID_PM25_MEASURED_VALUE      0x0000
#define ATTRID_PM10_MEASURED_VALUE      0x0601
#define ATTRID_PM100_MEASURED_VALUE     0x0602
#define ATTRID_AQI25_MEASURED_VALUE     0x0604
#define ATTRID_PWM_CORECTION_VALUE      0x0333
#define ATTRID_DISPLAY_INVERT           0xF004
#define ATTRID_AINP_PRESENT_VALUE       0x0055
#define ATTRID_AINP_A                   0x0175
#define ATTRID_AINP_B                   0x0185

/*********************************************************************
 * CONSTANTS - Level Control Default Values
 *********************************************************************/

#define ATTR_LEVEL_MAX_LEVEL            0xFE   // Maximum level
#define ATTR_LEVEL_MIN_LEVEL            0x01   // Minimum level
#define ATTR_LEVEL_MID_LEVEL            0x7E   // Mid level
#define ATTR_LEVEL_ON_LEVEL_NO_EFFECT   0xFF

#define DEFAULT_IDENTIFY_TIME           0
#define DEFAULT_ON_OFF_TRANSITION_TIME  20
#define DEFAULT_ON_LEVEL                ATTR_LEVEL_ON_LEVEL_NO_EFFECT
#define DEFAULT_ON_TRANSITION_TIME      20
#define DEFAULT_OFF_TRANSITION_TIME     20
#define DEFAULT_MOVE_RATE               0      // As fast as possible

/*********************************************************************
 * VARIABLES - Simple Descriptors
 *********************************************************************/

extern SimpleDescriptionFormat_t zclApp_FirstEP;
extern SimpleDescriptionFormat_t zclApp_SecEP;
extern SimpleDescriptionFormat_t zclApp_THEP;

/*********************************************************************
 * VARIABLES - Level Control Cluster Attributes
 *********************************************************************/

extern uint8 zclLevel_CurrentLevel;
extern uint16 zclLevel_LevelRemainingTime;
extern uint16 zclLevel_LevelOnOffTransitionTime;
extern uint8  zclLevel_LevelOnLevel;
extern uint16 zclLevel_LevelOnTransitionTime;
extern uint16 zclLevel_LevelOffTransitionTime;
extern uint8 zclLevel_LevelDefaultMoveRate;

/*********************************************************************
 * TYPEDEFS - Application Configuration Structure
 *********************************************************************/

typedef struct
{
    uint8 LightIndLevel;        // LED indicator brightness level
    uint8 LightIndLevel_Old;    // Previous brightness level
    bool LightIndicator;        // LED indicator enable/disable
    bool LightIndicator_Old;    // Previous indicator state
    int8 PWM_Corection;         // PWM correction value
    bool invert;                // Display color inversion flag
    bool invert_old;            // Previous inversion state
} application_config_t;

/*********************************************************************
 * TYPEDEFS - Sensor Data Structure
 *********************************************************************/

typedef struct {
    float pm10;                 // PM1.0 concentration (µg/m³)
    float pm25;                 // PM2.5 concentration (µg/m³)
    float pm100;                // PM10 concentration (µg/m³)
    float aqi25;                // Calculated AQI from PM2.5
    float aqi;                  // AQI from ENS160 sensor
    float tvoc;                 // TVOC concentration (ppb)
    float eco2;                 // eCO2 concentration (ppm)
} sensors_state_t;

/*********************************************************************
 * VARIABLES - Global Application Data
 *********************************************************************/

extern application_config_t zclApp_Config;
extern sensors_state_t zclApp_Sensors;
extern uint16 zclApp_IdentifyTime;
extern int16 sendInitReportCount;

/*********************************************************************
 * VARIABLES - Attribute Lists
 *********************************************************************/

extern CONST zclAttrRec_t zclApp_AttrsFirstEP[];
extern CONST zclAttrRec_t zclApp_AttrsSecEP[];
extern CONST zclAttrRec_t zclApp_AttrsTHEP[];

extern CONST uint8 zclApp_AttrsFirstEPCount;
extern CONST uint8 zclApp_AttrsSecEPCount;
extern CONST uint8 zclApp_AttrsTHEPCount;

/*********************************************************************
 * VARIABLES - Basic Cluster Attributes
 *********************************************************************/

extern const uint8 zclApp_ManufacturerName[];
extern const uint8 zclApp_ModelId[];
extern const uint8 zclApp_PowerSource;
extern const uint8 zclApp_DateCode[];
extern const uint8 zclApp_SWBuildID[];

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*
 * Initialization for the task
 */
extern void zclApp_Init(byte task_id);

/*
 * Event processing loop for the task
 */
extern UINT16 zclApp_event_loop(byte task_id, UINT16 events);

/*
 * Reset ZCL attributes to default values
 */
extern void zclApp_ResetAttributesToDefaultValues(void);

/*
 * On/Off cluster command callback
 */
static void zclApp_OnOffCB(uint8 cmd);

#ifdef __cplusplus
}
#endif

#endif /* ZCL_APP_H */
/*********************************************************************
 * ZCL_APP_DATA.C - ZCL Application Data (Attributes & Descriptors)
 *********************************************************************/

#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ms.h"
#include "zcl_ha.h"

#include "zcl_app.h"

/*********************************************************************
 * GLOBAL VARIABLES
 *********************************************************************/

// Cluster revision for all clusters
const uint16 zclApp_clusterRevision_all = 0x0001;

/*********************************************************************
 * APPLICATION CONFIGURATION (default values)
 *********************************************************************/

application_config_t zclApp_Config = {
    .LightIndLevel = 100,
    .LightIndLevel_Old = 100,
    .LightIndicator = true,
    .LightIndicator_Old = true,
    .PWM_Corection = -7,
    .invert = 0,
    .invert_old = 0,
};

/*********************************************************************
 * SENSOR DATA (default values)
 *********************************************************************/

sensors_state_t zclApp_Sensors = {
    .pm10 = 0.0,
    .pm25 = 0.0,
    .pm100 = 0.0,
    .aqi25 = 0.0,
    .aqi = 0,
    .tvoc = 0,
    .eco2 = 0,
};

/*********************************************************************
 * ZCL ATTRIBUTES
 *********************************************************************/

uint16 zclApp_IdentifyTime = DEFAULT_IDENTIFY_TIME;

// Level Control Cluster Attributes
extern uint8 zclLevel_CurrentLevel = 0;
extern uint16 zclLevel_LevelRemainingTime = 0;
extern uint16 zclLevel_LevelOnOffTransitionTime = DEFAULT_ON_OFF_TRANSITION_TIME;
extern uint8  zclLevel_LevelOnLevel = DEFAULT_ON_LEVEL;
extern uint16 zclLevel_LevelOnTransitionTime = DEFAULT_ON_TRANSITION_TIME;
extern uint16 zclLevel_LevelOffTransitionTime = DEFAULT_OFF_TRANSITION_TIME;
extern uint8 zclLevel_LevelDefaultMoveRate = DEFAULT_MOVE_RATE;

/*********************************************************************
 * DEVICE INFORMATION
 *********************************************************************/

#define APP_DEVICE_VERSION  4
#define APP_FLAGS           4

#define APP_HWVERSION       1
#define APP_ZCLVERSION      1

const uint8 zclApp_HWRevision = APP_HWVERSION;
const uint8 zclApp_ZCLVersion = APP_ZCLVERSION;
const uint8 zclApp_ApplicationVersion = 3;
const uint8 zclApp_StackVersion = 4;

/*********************************************************************
 * BASIC CLUSTER STRING ATTRIBUTES
 *********************************************************************/

// Manufacturer name: "EfektaLab_for_you"
const uint8 zclApp_ManufacturerName[] = {17, 'E', 'f', 'e', 'k', 't', 'a', 'L', 'a', 'b', '_', 'f', 'o', 'r', '_', 'y', 'o', 'u'};

// Model ID: "Open_PM_Monitor"
const uint8 zclApp_ModelId[] = {15, 'O', 'p', 'e', 'n', '_', 'P', 'M', '_', 'M', 'o', 'n', 'i', 't', 'o', 'r'};

// Power source: DC
const uint8 zclApp_PowerSource = POWER_SOURCE_DC;

// Date code: "20260320 643"
const uint8 zclApp_DateCode[] = {12, '2', '0', '2', '6', '0', '3', '2', '0', ' ', '6', '4', '3'};

// Software build ID: "1.1.6"
const uint8 zclApp_SWBuildID[] = {5, '1', '.', '1', '.', '6'};

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Endpoint 1 (Dimmable Light)
 *********************************************************************/

CONST zclAttrRec_t zclApp_AttrsFirstEP[] = {
    // Basic Cluster
    {BASIC, {ATTRID_BASIC_ZCL_VERSION, ZCL_UINT8, R, (void *)&zclApp_ZCLVersion}},
    {BASIC, {ATTRID_BASIC_APPL_VERSION, ZCL_UINT8, R, (void *)&zclApp_ApplicationVersion}},
    {BASIC, {ATTRID_BASIC_STACK_VERSION, ZCL_UINT8, R, (void *)&zclApp_StackVersion}},
    {BASIC, {ATTRID_BASIC_HW_VERSION, ZCL_UINT8, R, (void *)&zclApp_HWRevision}},   
    {BASIC, {ATTRID_BASIC_MANUFACTURER_NAME, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ManufacturerName}},
    {BASIC, {ATTRID_BASIC_MODEL_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ModelId}},
    {BASIC, {ATTRID_BASIC_DATE_CODE, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_DateCode}},
    {BASIC, {ATTRID_BASIC_POWER_SOURCE, ZCL_DATATYPE_ENUM8, R, (void *)&zclApp_PowerSource}},
    {BASIC, {ATTRID_BASIC_SW_BUILD_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_SWBuildID}},
    {BASIC, {ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, R, (void *)&zclApp_clusterRevision_all}},   
    
    // Identify Cluster
    {IDENTIFY, {ATTRID_IDENTIFY_TIME, ZCL_DATATYPE_UINT16, RW, (void *)&zclApp_IdentifyTime}},
    {IDENTIFY, {ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, R, (void *)&zclApp_clusterRevision_all}},
    
    // Level Control Cluster
    {ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL, {ATTRID_LEVEL_CURRENT_LEVEL, ZCL_DATATYPE_UINT8, RR, (void *)&zclLevel_CurrentLevel}},
    {ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL, {ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, R, (void *)&zclApp_clusterRevision_all}},  
    
    // On/Off Cluster
    {ZCL_CLUSTER_ID_GEN_ON_OFF, {ATTRID_ON_OFF, ZCL_DATATYPE_BOOLEAN, RR, (void *)&zclApp_Config.LightIndicator}},
    {ZCL_CLUSTER_ID_GEN_ON_OFF, {ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, R, (void *)&zclApp_clusterRevision_all}},   
};

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Endpoint 2 (PM Sensor)
 *********************************************************************/

CONST zclAttrRec_t zclApp_AttrsSecEP[] = {
    {ZCL_PM25, {ATTRID_PM25_MEASURED_VALUE, ZCL_SINGLE, RR, (void *)&zclApp_Sensors.pm25}},
    {ZCL_PM25, {ATTRID_PM10_MEASURED_VALUE, ZCL_SINGLE, RR, (void *)&zclApp_Sensors.pm10}},
    {ZCL_PM25, {ATTRID_PM100_MEASURED_VALUE, ZCL_SINGLE, RR, (void *)&zclApp_Sensors.pm100}},
    {ZCL_PM25, {ATTRID_AQI25_MEASURED_VALUE, ZCL_SINGLE, RR, (void *)&zclApp_Sensors.aqi25}},
    {ZCL_PM25, {ATTRID_PWM_CORECTION_VALUE, ZCL_DATATYPE_INT8, RW, (void *)&zclApp_Config.PWM_Corection}},
    {ZCL_PM25, {ATTRID_DISPLAY_INVERT, ZCL_DATATYPE_BOOLEAN, RW, (void *)&zclApp_Config.invert}},
};

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Endpoint 3 (ENS160 Sensor - TVOC/eCO2/AQI)
 *********************************************************************/

CONST zclAttrRec_t zclApp_AttrsTHEP[] = {
    {ANALOG_INPUT, {ATTRID_AINP_PRESENT_VALUE, ZCL_SINGLE, RR, (void *)&zclApp_Sensors.tvoc}},
    {ANALOG_INPUT, {ATTRID_AINP_A, ZCL_SINGLE, RR, (void *)&zclApp_Sensors.eco2}},
    {ANALOG_INPUT, {ATTRID_AINP_B, ZCL_SINGLE, RR, (void *)&zclApp_Sensors.aqi}},
};

/*********************************************************************
 * ATTRIBUTE LIST COUNTS
 *********************************************************************/

uint8 CONST zclApp_AttrsFirstEPCount = (sizeof(zclApp_AttrsFirstEP) / sizeof(zclApp_AttrsFirstEP[0]));
uint8 CONST zclApp_AttrsSecEPCount = (sizeof(zclApp_AttrsSecEP) / sizeof(zclApp_AttrsSecEP[0]));
uint8 CONST zclApp_AttrsTHEPCount = (sizeof(zclApp_AttrsTHEP) / sizeof(zclApp_AttrsTHEP[0]));

/*********************************************************************
 * INPUT CLUSTER LISTS
 *********************************************************************/

// Endpoint 1: Dimmable Light
const cId_t zclApp_InClusterList[] = {BASIC, IDENTIFY, ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL, ZCL_CLUSTER_ID_GEN_ON_OFF};

// Endpoint 2: PM Sensor
const cId_t zclApp_InClusterList2[] = {ZCL_PM25};

// Endpoint 3: ENS160 Sensor
const cId_t zclApp_InClusterList3[] = {ANALOG_INPUT};

#define APP_MAX_INCLUSTERS   (sizeof(zclApp_InClusterList) / sizeof(zclApp_InClusterList[0]))
#define APP_MAX_INCLUSTERS2  (sizeof(zclApp_InClusterList2) / sizeof(zclApp_InClusterList2[0]))
#define APP_MAX_INCLUSTERS3  (sizeof(zclApp_InClusterList3) / sizeof(zclApp_InClusterList3[0]))

/*********************************************************************
 * SIMPLE DESCRIPTORS
 *********************************************************************/

// Endpoint 1 - Dimmable Light
SimpleDescriptionFormat_t zclApp_FirstEP = {
    1,                                                  // Endpoint
    ZCL_HA_PROFILE_ID,                                  // Application Profile ID
    ZCL_HA_DEVICEID_DIMMABLE_LIGHT,                     // Device ID
    APP_DEVICE_VERSION,                                 // Device Version
    APP_FLAGS,                                          // Flags
    APP_MAX_INCLUSTERS,                                 // Number of Input Clusters
    (cId_t *)zclApp_InClusterList,                      // Input Cluster List
    0,                                                  // Number of Output Clusters
    (cId_t *)NULL,                                      // Output Cluster List
};

// Endpoint 2 - PM Sensor
SimpleDescriptionFormat_t zclApp_SecEP = {
    2,                                                  // Endpoint
    ZCL_HA_PROFILE_ID,                                  // Application Profile ID
    ZCL_HA_DEVICEID_SIMPLE_SENSOR,                      // Device ID
    APP_DEVICE_VERSION,                                 // Device Version
    APP_FLAGS,                                          // Flags
    APP_MAX_INCLUSTERS2,                                // Number of Input Clusters
    (cId_t *)zclApp_InClusterList2,                     // Input Cluster List
    0,                                                  // Number of Output Clusters
    (cId_t *)NULL,                                      // Output Cluster List
};

// Endpoint 3 - ENS160 Sensor
SimpleDescriptionFormat_t zclApp_THEP = {
    3,                                                  // Endpoint
    ZCL_HA_PROFILE_ID,                                  // Application Profile ID
    ZCL_HA_DEVICEID_SIMPLE_SENSOR,                      // Device ID
    APP_DEVICE_VERSION,                                 // Device Version
    APP_FLAGS,                                          // Flags
    APP_MAX_INCLUSTERS3,                                // Number of Input Clusters
    (cId_t *)zclApp_InClusterList3,                     // Input Cluster List
    0,                                                  // Number of Output Clusters
    (cId_t *)NULL,                                      // Output Cluster List
};

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      zclApp_ResetAttributesToDefaultValues
 * @brief   Reset ZCL attributes to default values
 *********************************************************************/

void zclApp_ResetAttributesToDefaultValues(void)
{
    zclApp_IdentifyTime = DEFAULT_IDENTIFY_TIME;    
}
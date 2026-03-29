/*********************************************************************
 * INCLUDES
 *********************************************************************/

#include "AF.h"
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "OSAL_PwrMgr.h"
#include "ZComDef.h"
#include "ZDApp.h"
#include "ZDNwkMgr.h"
#include "ZDObject.h"
#include "math.h"

#include "nwk_util.h"
#include "zcl.h"
#include "zcl_app.h"
#include "zcl_diagnostic.h"
#include "zcl_general.h"
#include "zcl_lighting.h"
#include "zcl_ms.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "gp_interface.h"

#ifdef ZCL_LEVEL_CTRL
#include "zcl_ll.h"
#endif
#include "OnBoard.h"

/* HAL */
#include "hal_adc.h"
#include "hal_drivers.h"
#include "hal_key.h"
#include "hal_led.h"

#include "commissioning.h"
#include "factory_reset.h"
#include "utils.h"
#include "hal_i2c.h"

#include "pm_sensor_driver.h"

#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
#include "zcl_ota.h"
#include "hal_ota.h"
#endif

#include "led_task.h"
#include "pwm_rgb.h"

#include "OLED_1306.h"
#include "imagedata.h"

#include "ens160.h"

/*********************************************************************
 * MACROS
 *********************************************************************/

#define HAL_KEY_CODE_RELEASE_KEY HAL_KEY_CODE_NOKEY
#define myround(x) ((int)((x)+0.5))

/*********************************************************************
 * CONSTANTS
 *********************************************************************/

#define PWM_FREQUENCY 125000
#define RED_CHANNEL   4
#define GREEN_CHANNEL 3
#define BLUE_CHANNEL  2

/*********************************************************************
 * GLOBAL VARIABLES
 *********************************************************************/

extern bool requestNewTrustCenterLinkKey;
byte zclApp_TaskID;

/*********************************************************************
 * LOCAL VARIABLES
 *********************************************************************/

static bool identifyStart = 0;
static bool allRep = false;
static uint8 currentSensorsReadingPhase = 0;
static uint8 currentSensorsSendingPhase = 0;
static bool initOut = false;
static int16 sendInitReportCount = 0;
static uint8 lengthPM = 0;
static uint8 current_rgb_red = 0;
static uint8 current_rgb_green = 0;
static uint8 current_rgb_blue = 0;
static bool force_led_update = false;
static SSD1306_COLOR currentDisplayColor = White;
static SSD1306_COLOR currentBackgroundDisplayColor = Black;
static uint8 oled_display_mode = 0;  // 0 = PM, 1 = TVOC, 2 = eCO2

// OLED display presence flag
static bool displayPresent = false;
static bool ens160Present = false;  // ENS160 sensor presence flag

uint8 SeqNum = 0x00;
afAddrType_t inderect_DstAddr = {.addrMode = (afAddrMode_t)AddrNotPresent, .endPoint = 0, .addr.shortAddr = 0};

/*********************************************************************
 * LOCAL FUNCTIONS PROTOTYPES
 *********************************************************************/

static float zclApp_PM2_5_TO_AQI(float PM);
static void zclApp_HandleKeys(byte shift, byte keys);
static void zclApp_Report(void);
static void zclApp_BasicResetCB(void);
static void zclApp_RestoreAttributesFromNV(void);
static void zclApp_SaveAttributesToNV(void);
static void zclApp_ReadAPM(void);
static ZStatus_t zclApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper);
static void zclApp_ReadSensors(void);
static void zclApp_SendSensors(void);
static void zclApp_PM1_Send(void);
static void zclApp_PM25_Send(void);
static void zclApp_PM10_Send(void);
static void zclApp_IQIND25_Send(void);
static void zclApp_PWM_Cor_Send(void);
static void zclApp_PWM_Invert_Send(void);
static void zclApp_LedOn(void);
static void zclApp_LedOff(void);
static void zclApp_rgbRun(void);
static void zclApp_OnOff_Send(void);
static void zclApp_Level_Send(void);
static void zclApp_ProcessIdentifyTimeChange(uint8 endpoint);
void zclLevel_MoveToLevelCB(zclLCMoveToLevel_t *pCmd);
void DelayMs(unsigned int delaytime);
void identifyLight(void);
uint8_t map_value(uint8_t input, uint8_t in_min, uint8_t in_max, uint8_t out_min, uint8_t out_max);
void PWMka_initTimer1(void);
void PWMka(uint32 freq, uint16 tduty, uint8 channal);
static bool oled_Detect(void);

#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
static void zclApp_ProcessOTAMsgs(zclOTA_CallbackMsg_t* pMsg);
#endif

void oledGo(void);
void oledRefresh(void);
void oledSetColor(bool invert);
void oledPMdata(uint16 pmLevel);
void oledTVOCdata(uint16 tvocLevel);
void oledCO2data(uint16 co2Level);

void readVOC(void);
static void zclApp_TVOC_Send(void);

static bool ens160_Detect(void);

/*********************************************************************
 * AQI Constants (EPA Standard)
 *********************************************************************/

const float I_hi[7] = {50.0, 100.0, 150.0, 200.0, 300.0, 400.0, 500.0};
const float I_lo[7] = {0.0, 51.0, 101.0, 151.0, 201.0, 301.0, 401.0};
const float BP_hi_2_5[7] = {12.0, 35.4, 55.4, 150.4, 250.4, 350.4, 500.4};
const float BP_lo_2_5[7] = {0.0, 12.1, 35.5, 55.5, 150.5, 250.5, 350.5};

/*********************************************************************
 * ZCL General Profile Callback table
 *********************************************************************/

static zclGeneral_AppCallbacks_t zclApp_CmdCallbacks = {
    zclApp_BasicResetCB,            // Basic Cluster Reset command
    NULL,                           // Identify Trigger Effect command
    zclApp_OnOffCB,                 // On/Off cluster commands
    NULL,                           // On/Off cluster enhanced command Off with Effect
    NULL,                           // On/Off cluster enhanced command On with Recall Global Scene
    NULL,                           // On/Off cluster enhanced command On with Timed Off
#ifdef ZCL_LEVEL_CTRL
    zclLevel_MoveToLevelCB,         // Level Control Move to Level command
    NULL,                           // Level Control Move command
    NULL,                           // Level Control Step command
    NULL,                           // Level Control Stop command    
#endif
#ifdef ZCL_GROUPS
    NULL,                           // Group Response commands
#endif
#ifdef ZCL_SCENES
    NULL,                           // Scene Store Request command
    NULL,                           // Scene Recall Request command
    NULL,                           // Scene Response command
#endif
#ifdef ZCL_ALARMS
    NULL,                           // Alarm (Response) commands
#endif
#ifdef SE_UK_EXT
    NULL,                           // Get Event Log command
    NULL,                           // Publish Event Log command
#endif    
    NULL,                           // RSSI Location command
    NULL                            // RSSI Location Response command
};

/*********************************************************************
 * @fn      oled_Detect
 * @brief   Detect if OLED display is connected by checking I2C ACK
 * @return  true if display detected, false otherwise
 *********************************************************************/

static bool oled_Detect(void) {
    uint8_t test_byte = 0xAE;  // Display OFF command (safe command)
    int8_t result;
    
    // I2C already initialized by sensor_init()
    // Try to send a simple command to the OLED display at address 0x3C
    // If the device responds with ACK, I2C_WriteMultByte will return I2C_SUCCESS (0)
    // If no device at this address, it will return I2C_ERROR (1)
    
    // Send command to OLED: control byte 0x00 (command mode) + command 0xAE
    // Using I2C_WriteMultByte which is the same function used by ssd1306_WriteCommand
    result = I2C_WriteMultByte(OLED_ADDR, 0x00, &test_byte, 1);
    
    // Return true if write succeeded (display present), false otherwise
    return (result == I2C_SUCCESS);
}

/*********************************************************************
 * @fn      zclApp_Init
 *
 * @brief   Initialization function for the ZCL Application task
 *********************************************************************/

void zclApp_Init(byte task_id) {
    HAL_TURN_ON_LED1();
    DelayMs(500);
    HAL_TURN_OFF_LED1();
    
    zclApp_RestoreAttributesFromNV();
    
    PWMka_initTimer1();
    PWMka(PWM_FREQUENCY, 0, BLUE_CHANNEL);
    PWMka(PWM_FREQUENCY, 0, GREEN_CHANNEL);
    PWMka(PWM_FREQUENCY, 0, RED_CHANNEL);
    
    uint16 start_arrow = 64;
    // Apply PWM correction (same logic as in zclApp_rgbRun)
    int16 start_with_shift = start_arrow - (int16)zclApp_Config.PWM_Corection;
    
    if (start_with_shift < 1) start_with_shift = 1;
    if (start_with_shift > 127) start_with_shift = 127;
    
    pwm_rgb_set_channel(RETRO_VOLTMETER_CHANNEL, (uint8)start_with_shift);
    led_task_set_arrow_start((uint8)start_with_shift);
    
    DelayMs(50);
    zclLevel_CurrentLevel = zclApp_Config.LightIndLevel * 2;
    
    zclApp_TaskID = task_id;
    
    zclGeneral_RegisterCmdCallbacks(1, &zclApp_CmdCallbacks);
    zcl_registerAttrList(zclApp_FirstEP.EndPoint, zclApp_AttrsFirstEPCount, zclApp_AttrsFirstEP);
    bdb_RegisterSimpleDescriptor(&zclApp_FirstEP);
    zcl_registerReadWriteCB(zclApp_FirstEP.EndPoint, NULL, zclApp_ReadWriteAuthCB);
    
    zclGeneral_RegisterCmdCallbacks(2, &zclApp_CmdCallbacks);
    zcl_registerAttrList(zclApp_SecEP.EndPoint, zclApp_AttrsSecEPCount, zclApp_AttrsSecEP);
    bdb_RegisterSimpleDescriptor(&zclApp_SecEP);
    zcl_registerReadWriteCB(zclApp_SecEP.EndPoint, NULL, zclApp_ReadWriteAuthCB);
    
    zclGeneral_RegisterCmdCallbacks(3, &zclApp_CmdCallbacks);
    zcl_registerAttrList(zclApp_THEP.EndPoint, zclApp_AttrsTHEPCount, zclApp_AttrsTHEP);
    bdb_RegisterSimpleDescriptor(&zclApp_THEP);
    zcl_registerReadWriteCB(zclApp_THEP.EndPoint, NULL, zclApp_ReadWriteAuthCB);
    
    zcl_registerForMsg(zclApp_TaskID);
    bdb_RegisterIdentifyTimeChangeCB(zclApp_ProcessIdentifyTimeChange);
    
#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
    zclOTA_Register(zclApp_TaskID);
#endif
    
    RegisterForKeys(zclApp_TaskID);
    
    sensor_init();
    DelayMs(200);
    
    // ========== OLED DISPLAY DETECTION ==========
    // Detect if OLED display is present (I2C already initialized by sensor_init)
    displayPresent = oled_Detect();
    
    // Blue LED startup animation
    for (uint8 j = 127; j > 1; j--) {
        PWMka(PWM_FREQUENCY, j, BLUE_CHANNEL);
        DelayMs(20);
    }
    
    if (displayPresent) {
        // Display detected - initialize and show logo
        oledSetColor(zclApp_Config.invert);
        ssd1306_Init();
        ssd1306_SetContrast(0x00);
        ssd1306_Fill(currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(0, 0, logo, 128, 64, currentDisplayColor);
        ssd1306_UpdateScreen();
        
        DelayMs(500);
        
        // Fade-in animation
        uint8 st;
        uint32 mss = 100;
        for (st = 1; st < 200; st++) {
            ssd1306_SetContrast(st);
            DelayMs(mss);
            mss--;
            mss--;
            mss--;
            mss--;
            mss--;
            if (mss == 10) {
                mss = 15;
            }
        }
    }
    
    DelayMs(200);
    
    // Initialize ENS160 sensor
    ens160_init();
    ens160Present = ens160_Detect();
    
    // Continue with LED animation regardless of display
    for (uint8 j = 1; j < 127; j++) {
        PWMka(PWM_FREQUENCY, j, BLUE_CHANNEL);
        DelayMs(20);
    }
    PWMka(PWM_FREQUENCY, 0, BLUE_CHANNEL);
    
    HAL_TURN_ON_LED1();
    DelayMs(1000);
    HAL_TURN_OFF_LED1();
    
    // Clear display if present
    if (displayPresent) {
        ssd1306_Fill(currentBackgroundDisplayColor);
        ssd1306_UpdateScreen();
    }
    
    zclApp_Report();
    osal_start_reload_timer(zclApp_TaskID, APP_REPORT_EVT, APP_REPORT_DELAY);
}

/*********************************************************************
 * @fn      identifyLight
 *
 * @brief   Blink LED for identification
 *********************************************************************/

void identifyLight(void) {
    for (uint8 j = 127; j > 1; j--) {
        PWMka(PWM_FREQUENCY, j, BLUE_CHANNEL);
        DelayMs(10);
    }
    for (uint8 j = 1; j < 127; j++) {
        PWMka(PWM_FREQUENCY, j, BLUE_CHANNEL);
        DelayMs(10);
    }
    osal_start_timerEx(zclApp_TaskID, APP_IDENT_EVT, 200);
}

/*********************************************************************
 * @fn      zclApp_event_loop
 *
 * @brief   Event processing loop for ZCL Application
 *********************************************************************/

uint16 zclApp_event_loop(uint8 task_id, uint16 events) {
    afIncomingMSGPacket_t *MSGpkt;
    
    (void)task_id;
    
    if (events & SYS_EVENT_MSG) {
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclApp_TaskID))) {
            switch (MSGpkt->hdr.event) {
            case KEY_CHANGE:
                zclApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                break;
                
            case ZDO_STATE_CHANGE:
                // Network state change - handled elsewhere
                break;
                
#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
            case ZCL_OTA_CALLBACK_IND:
                zclApp_ProcessOTAMsgs((zclOTA_CallbackMsg_t*)MSGpkt);
                break;
#endif
                
            case ZCL_INCOMING_MSG:
                if (((zclIncomingMsg_t *)MSGpkt)->attrCmd) {
                    osal_mem_free(((zclIncomingMsg_t *)MSGpkt)->attrCmd);
                }
                break;
                
            default:
                break;
            }
            osal_msg_deallocate((uint8 *)MSGpkt);
        }
        return (events ^ SYS_EVENT_MSG);
    }
    
    if (events & APP_REPORT_EVT) {
        if (initOut == false) {
            sendInitReportCount++;
            if (sendInitReportCount == 5) { 
                initOut = true;
            }
            allRep = true;
            zclApp_Report();
        } else {
            zclApp_Report();
        }
        return (events ^ APP_REPORT_EVT);
    }
    
    if (events & APP_READ_SENSORS_EVT) {
        zclApp_ReadSensors();
        return (events ^ APP_READ_SENSORS_EVT);
    }
    
    if (events & APP_SEND_SENSORS_EVT) {
        zclApp_SendSensors();
        return (events ^ APP_SEND_SENSORS_EVT);
    }
    
    if (events & APP_SAVE_ATTRS_EVT) {
        zclApp_SaveAttributesToNV();
        return (events ^ APP_SAVE_ATTRS_EVT);
    }
    
    if (events & APP_LED_EVT) {
        zclApp_LedOff();
        return (events ^ APP_LED_EVT);
    }
    
    if (events & APP_IDENT_EVT) {
        identifyLight();
        return (events ^ APP_IDENT_EVT);
    }
    
    return 0;
}

/*********************************************************************
 * @fn      zclApp_LedOn / zclApp_LedOff
 *
 * @brief   LED control functions
 *********************************************************************/

static void zclApp_LedOn(void) {
    HAL_TURN_ON_LED1();
    osal_start_timerEx(zclApp_TaskID, APP_LED_EVT, 30);
}

static void zclApp_LedOff(void) {
    HAL_TURN_OFF_LED1();
}

/*********************************************************************
 * @fn      zclApp_HandleKeys
 *
 * @brief   Handle key press events
 *********************************************************************/

static void zclApp_HandleKeys(byte portAndAction, byte keyCode) {
#if APP_COMMISSIONING_BY_LONG_PRESS == TRUE
    if (bdbAttributes.bdbNodeIsOnANetwork == 1) {
        zclFactoryResetter_HandleKeys(portAndAction, keyCode);
    }
#else
    zclFactoryResetter_HandleKeys(portAndAction, keyCode);
#endif
    
    zclCommissioning_HandleKeys(portAndAction, keyCode);
    
    if (portAndAction & HAL_KEY_RELEASE) {
#ifdef OUTDOOR_LONG_RANGE    
        ZMacSetTransmitPower(TX_PWR_PLUS_19);
#else
        ZMacSetTransmitPower(TX_PWR_PLUS_4);
#endif    
        allRep = true;
        zclApp_Report();
    }
}

/*********************************************************************
 * @fn      zclApp_ReadSensors
 *
 * @brief   Read sensors in phases to avoid blocking
 *********************************************************************/

static void zclApp_ReadSensors(void) {
    switch (currentSensorsReadingPhase++) {
    case 0:
        zclApp_ReadAPM();
        readVOC();
        break;
        
    case 1:
        zclApp_rgbRun();
        oledRefresh();
        break;
        
    default:
        osal_start_timerEx(zclApp_TaskID, APP_SEND_SENSORS_EVT, 100);
        currentSensorsReadingPhase = 0;
        break;
    }
    
    if (currentSensorsReadingPhase != 0) {
        osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 100);
    }
}

/*********************************************************************
 * @fn      zclApp_SendSensors
 *
 * @brief   Send sensor data in phases to avoid blocking
 *********************************************************************/

static void zclApp_SendSensors(void) {
    switch (currentSensorsSendingPhase++) {  
    case 0:
        zclApp_PM25_Send();
        break;
        
    case 1:
        zclApp_PM1_Send();
        break;
        
    case 2:
        zclApp_PM10_Send();
        break;
        
    case 3:
        zclApp_IQIND25_Send();
        zclApp_TVOC_Send();
        break;
            
    default:
        currentSensorsSendingPhase = 0;
        
        if (allRep == true) {
            zclApp_OnOff_Send();
            zclApp_Level_Send();
            zclApp_PWM_Cor_Send();
            zclApp_PWM_Invert_Send();
            allRep = false;
        }
        break;
    }
    
    if (currentSensorsSendingPhase != 0) {
        osal_start_timerEx(zclApp_TaskID, APP_SEND_SENSORS_EVT, 100);
    }
}

/*********************************************************************
 * @fn      zclApp_Report
 *
 * @brief   Trigger sensor reading
 *********************************************************************/

static void zclApp_Report(void) {
    if (identifyStart == 0) {
        if (devState == DEV_ROUTER && allRep == true || devState == DEV_END_DEVICE && allRep == true) {
            zclApp_LedOn();
        }
        
        if (sendInitReportCount == 0) {
            initOut = false;
        }
        
        osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 100);
    }
}

/*********************************************************************
 * @fn      zclApp_BasicResetCB
 *
 * @brief   Basic Cluster Reset callback
 *********************************************************************/

static void zclApp_BasicResetCB(void) {
    // Reset handling - currently empty
}

/*********************************************************************
 * @fn      zclApp_ReadWriteAuthCB
 *
 * @brief   Read/Write authorization callback
 *********************************************************************/

static ZStatus_t zclApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper) {
    osal_start_timerEx(zclApp_TaskID, APP_SAVE_ATTRS_EVT, 2200);
    return ZSuccess;
}

/*********************************************************************
 * @fn      zclApp_SaveAttributesToNV
 *
 * @brief   Save configuration to non-volatile memory
 *********************************************************************/

static void zclApp_SaveAttributesToNV(void) {
    osal_nv_write(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
    
    if (zclApp_Config.invert != zclApp_Config.invert_old) {
        zclApp_Config.invert_old = zclApp_Config.invert;
        oledSetColor(zclApp_Config.invert);
        if (displayPresent) {
            ssd1306_Fill(currentBackgroundDisplayColor);
            ssd1306_UpdateScreen();
            oledRefresh();
        }
    }
}

/*********************************************************************
 * @fn      zclApp_RestoreAttributesFromNV
 *
 * @brief   Restore configuration from non-volatile memory
 *********************************************************************/

static void zclApp_RestoreAttributesFromNV(void) {
    uint8 status = osal_nv_item_init(NW_APP_CONFIG, sizeof(application_config_t), NULL);
    
    if (status == NV_ITEM_UNINIT) {
        osal_nv_write(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
    }
    
    if (status == ZSUCCESS) {
        osal_nv_read(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
    }
}

/*********************************************************************
 * @fn      DelayMs
 *
 * @brief   Simple millisecond delay
 *********************************************************************/

void DelayMs(unsigned int delaytime) {
    while (delaytime--) {
        uint16 microSecs = 1000;
        while (microSecs--) {
            asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
        }
    }
}

/*********************************************************************
 * @fn      zclApp_ReadAPM
 *
 * @brief   Read particulate matter sensor data
 *********************************************************************/

static void zclApp_ReadAPM(void) {
    uint16 pm1_t = 100;
    uint16 pm2_5_t = 100;
    uint16 pm10_t = 100;
    
    if (sensor_read(&pm1_t, &pm2_5_t, &pm10_t)) {
        zclApp_Sensors.pm10 = (float)pm1_t;
        zclApp_Sensors.pm25 = (float)pm2_5_t;
        zclApp_Sensors.pm100 = (float)pm10_t;
        
        zclApp_Sensors.aqi25 = myround(zclApp_PM2_5_TO_AQI(zclApp_Sensors.pm25));
        if (zclApp_Sensors.aqi25 > 500) {
            zclApp_Sensors.aqi25 = 500;
        }
    }
}

/*********************************************************************
 * PM Sensor Send Functions
 *********************************************************************/

static void zclApp_PM1_Send(void) {
    if (allRep == true) {
        const uint8 NUM_ATTRIBUTES = 1;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
        
        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;
            pReportCmd->attrList[0].attrID = ATTRID_PM10_MEASURED_VALUE;
            pReportCmd->attrList[0].dataType = ZCL_SINGLE;
            pReportCmd->attrList[0].attrData = (void *)(&zclApp_Sensors.pm10);
            
            zcl_SendReportCmd(2, &inderect_DstAddr, ZCL_PM25, pReportCmd, ZCL_FRAME_SERVER_CLIENT_DIR, true, SeqNum++);
            osal_mem_free(pReportCmd);
        }
    } else {
        bdb_RepChangedAttrValue(2, ZCL_PM25, ATTRID_PM10_MEASURED_VALUE);
    }
}

static void zclApp_PM25_Send(void) {
    if (allRep == true) {
        const uint8 NUM_ATTRIBUTES = 1;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
        
        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;
            pReportCmd->attrList[0].attrID = ATTRID_PM25_MEASURED_VALUE;
            pReportCmd->attrList[0].dataType = ZCL_SINGLE;
            pReportCmd->attrList[0].attrData = (void *)(&zclApp_Sensors.pm25);
            
            zcl_SendReportCmd(2, &inderect_DstAddr, ZCL_PM25, pReportCmd, ZCL_FRAME_SERVER_CLIENT_DIR, true, SeqNum++);
            osal_mem_free(pReportCmd);
        }
    } else {
        bdb_RepChangedAttrValue(2, ZCL_PM25, ATTRID_PM25_MEASURED_VALUE);
    }
}

static void zclApp_PM10_Send(void) {
    if (allRep == true) {
        const uint8 NUM_ATTRIBUTES = 1;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
        
        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;
            pReportCmd->attrList[0].attrID = ATTRID_PM100_MEASURED_VALUE;
            pReportCmd->attrList[0].dataType = ZCL_SINGLE;
            pReportCmd->attrList[0].attrData = (void *)(&zclApp_Sensors.pm100);
            
            zcl_SendReportCmd(2, &inderect_DstAddr, ZCL_PM25, pReportCmd, ZCL_FRAME_SERVER_CLIENT_DIR, true, SeqNum++);
            osal_mem_free(pReportCmd);
        }
    } else {
        bdb_RepChangedAttrValue(2, ZCL_PM25, ATTRID_PM100_MEASURED_VALUE);
    }
}

static void zclApp_IQIND25_Send(void) {
    if (allRep == true) {
        const uint8 NUM_ATTRIBUTES = 1;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
        
        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;
            pReportCmd->attrList[0].attrID = ATTRID_AQI25_MEASURED_VALUE;
            pReportCmd->attrList[0].dataType = ZCL_SINGLE;
            pReportCmd->attrList[0].attrData = (void *)(&zclApp_Sensors.aqi25);
            
            zcl_SendReportCmd(2, &inderect_DstAddr, ZCL_PM25, pReportCmd, ZCL_FRAME_SERVER_CLIENT_DIR, true, SeqNum++);
            osal_mem_free(pReportCmd);
        }
    } else {
        bdb_RepChangedAttrValue(2, ZCL_PM25, ATTRID_AQI25_MEASURED_VALUE);
    }
}

static void zclApp_PWM_Cor_Send(void) {
    if (allRep == true) {
        const uint8 NUM_ATTRIBUTES = 1;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
        
        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;
            pReportCmd->attrList[0].attrID = ATTRID_PWM_CORECTION_VALUE;
            pReportCmd->attrList[0].dataType = ZCL_DATATYPE_INT8;
            pReportCmd->attrList[0].attrData = (void *)(&zclApp_Config.PWM_Corection);
            
            zcl_SendReportCmd(2, &inderect_DstAddr, ZCL_PM25, pReportCmd, ZCL_FRAME_SERVER_CLIENT_DIR, true, SeqNum++);
            osal_mem_free(pReportCmd);
        }
    }
}

static void zclApp_PWM_Invert_Send(void) {
    if (allRep == true) {
        const uint8 NUM_ATTRIBUTES = 1;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
        
        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;
            pReportCmd->attrList[0].attrID = ATTRID_DISPLAY_INVERT;
            pReportCmd->attrList[0].dataType = ZCL_DATATYPE_BOOLEAN;
            pReportCmd->attrList[0].attrData = (void *)(&zclApp_Config.invert);
            
            zcl_SendReportCmd(2, &inderect_DstAddr, ZCL_PM25, pReportCmd, ZCL_FRAME_SERVER_CLIENT_DIR, true, SeqNum++);
            osal_mem_free(pReportCmd);
        }
    }
}

static void zclApp_TVOC_Send(void) {
    if (!ens160Present) {
        return;
    }
    
    if (allRep == true) {
        const uint8 NUM_ATTRIBUTES = 3;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
        
        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;
            
            pReportCmd->attrList[0].attrID = ATTRID_AINP_PRESENT_VALUE;
            pReportCmd->attrList[0].dataType = ZCL_SINGLE;
            pReportCmd->attrList[0].attrData = (void *)(&zclApp_Sensors.tvoc);
            
            pReportCmd->attrList[1].attrID = ATTRID_AINP_A;
            pReportCmd->attrList[1].dataType = ZCL_SINGLE;
            pReportCmd->attrList[1].attrData = (void *)(&zclApp_Sensors.eco2);
            
            pReportCmd->attrList[2].attrID = ATTRID_AINP_B;
            pReportCmd->attrList[2].dataType = ZCL_SINGLE;
            pReportCmd->attrList[2].attrData = (void *)(&zclApp_Sensors.aqi);
            
            zcl_SendReportCmd(3, &inderect_DstAddr, ANALOG_INPUT, pReportCmd, ZCL_FRAME_SERVER_CLIENT_DIR, true, SeqNum++);
            osal_mem_free(pReportCmd);
        }
    } else {
        bdb_RepChangedAttrValue(3, ANALOG_INPUT, ATTRID_AINP_PRESENT_VALUE);
    }
}

/*********************************************************************
 * @fn      zclApp_PM2_5_TO_AQI
 *
 * @brief   Convert PM2.5 concentration to AQI value (EPA standard)
 *********************************************************************/

static float zclApp_PM2_5_TO_AQI(float PM) {
    if (PM < 0) return -1;
    
    int d = -1;
    for (int i = 0; i < 7; i++) {
        d = i;
        if (PM <= BP_hi_2_5[i]) {
            break;
        }
    }
    
    return (I_hi[d] - I_lo[d]) / (BP_hi_2_5[d] - BP_lo_2_5[d]) * (PM - BP_lo_2_5[d]) + I_lo[d] + 0.5;
}

/*********************************************************************
 * PWM Functions
 *********************************************************************/

void PWMka_initTimer1(void) {
    P0DIR |= 0x78;          // P0.3, P0.4, P0.5, P0.6 as output
    PERCFG &= ~0x40;        // Select alternative 1 for Timer 1
    PERCFG |= 0x03;         // Select USART0, USART1 alternative 2
    P2DIR &= ~0xC0;         // Timer 1 channels all have priority
    P0SEL |= 0x78;          // P0.3, P0.4, P0.5, P0.6 as peripheral
}

void PWMka(uint32 freq, uint16 tduty, uint8 channal) {
    T1CTL &= ~(BV(1) | BV(0));      // Suspend Timer 1 operation
    
    uint16 tfreq;
    if (freq <= 32768) {
        tfreq = (uint16)(2000000 / freq);
    } else {
        tfreq = (uint16)(16000000 / freq);
    }
    
    // PWM signal period
    T1CC0L = (uint8)tfreq;
    T1CC0H = (uint8)(tfreq >> 8);
    
    // PWM Duty Cycle
    switch (channal) {
        case 1:
            T1CC1H = (uint8)(tduty >> 8);
            T1CC1L = (uint8)tduty;
            T1CCTL1 = 0x1C;
            break;
        case 2:
            T1CC2H = (uint8)(tduty >> 8);
            T1CC2L = (uint8)tduty;
            T1CCTL2 = 0x1C;
            break;
        case 3:
            T1CC3H = (uint8)(tduty >> 8);
            T1CC3L = (uint8)tduty;
            T1CCTL3 = 0x1C;
            break;
        case 4:
            T1CC4H = (uint8)(tduty >> 8);
            T1CC4L = (uint8)tduty;
            T1CCTL4 = 0x1C;
            break;
        default:
            break;
    }
    
    if (freq <= 32768) {
        T1CTL |= (BV(2) | 0x03);    // Up-down mode, clock / 8
    } else {
        T1CTL = 0x03;               // Up-down mode, clock / 1
    }
}

/*********************************************************************
 * @fn      zclApp_rgbRun
 *
 * @brief   Update LED indicator based on AQI value
 *********************************************************************/

static void zclApp_rgbRun(void) {
    static uint8 last_red = 0;
    static uint8 last_green = 0;
    static uint8 last_blue = 0;
    static uint16 last_indicator = 128;
    static bool first_run = true;
    static int8 last_pwm_correction = 0;
    
    // ========== Calculate AQI value ==========
    // PM2.5 AQI (0-500)
    uint16 pm_aqi = 0;
    uint16 tvoc_aqi = 0;
      
    pm_aqi = (uint16)zclApp_Sensors.aqi25;
    if (pm_aqi > 500) pm_aqi = 500;
    
    // TVOC -> AQI (0-500)
    if (ens160Present) {
        uint16 tvoc = (uint16)zclApp_Sensors.tvoc;
        if (tvoc > 9999) tvoc = 9999;
        tvoc_aqi = (uint16)(((uint32)tvoc * 500) / 9999);
    }
    
    // Select maximum value (conservative approach)
    uint16 aqi_value = (tvoc_aqi > pm_aqi) ? tvoc_aqi : pm_aqi;
    
    // ========== Arrow position ==========
    // Physical range of the arrow mechanism (6-122)
    float min = 6.0f;
    float max = 122.0f;
    
    // PWM correction shift
    float shift = (float)zclApp_Config.PWM_Corection;
    
    // Raw mapping of AQI to physical range
    uint16 indicator_raw = (uint16)mapRange(500, 0, min, max, aqi_value);
    
    // Apply shift
    int16 new_indicator = (int16)indicator_raw - (int16)shift;
    
    // Clamping
    if (new_indicator < 1) new_indicator = 1;
    if (new_indicator > 127) new_indicator = 127;
    
    // Check if PWM correction changed
    bool correction_changed = (zclApp_Config.PWM_Corection != last_pwm_correction);
    
    if (!correction_changed) {
        if ((uint8)new_indicator != last_indicator) {
            led_task_send_arrow((uint8)new_indicator);
            last_indicator = (uint8)new_indicator;
        }
    } else {
        // If correction changed - update immediately
        led_task_set_arrow_immediate((uint8)new_indicator);
        last_indicator = (uint8)new_indicator;
        last_pwm_correction = zclApp_Config.PWM_Corection;
    }
    
    // ========== RGB LED ==========
    if (zclApp_Config.LightIndicator == false) {
        led_message_t msg = {LED_CMD_SET_COLOR, 0, 0, 0, 0, 0};
        led_task_send_command(&msg);
        current_rgb_red = current_rgb_green = current_rgb_blue = 0;
        if (displayPresent) ssd1306_SetContrast(2);
        first_run = true;
        return;
    }
    
    uint8 level = (zclApp_Config.LightIndLevel == 0) ? 127 : map_value(zclApp_Config.LightIndLevel, 1, 127, 1, 127);
    uint8 brightness = (level == 0) ? 0 : 128 - level;
    
    uint8 zone = (aqi_value < 110) ? 2 : (aqi_value < 250) ? 3 : (aqi_value < 380) ? 4 : 5;
    
    uint8 red = 0, green = 0, blue = 0;
    switch (zone) {
        case 2: green = brightness; break;
        case 3: red = brightness; green = brightness; break;
        case 4: red = brightness; blue = brightness; break;
        case 5: red = brightness; break;
    }
    
    current_rgb_red = red;
    current_rgb_green = green;
    current_rgb_blue = blue;
    
    if (force_led_update) {
        last_red = last_green = last_blue = 0;
        force_led_update = false;
    }
    
    if (first_run) {
        led_message_t msg = {LED_CMD_SET_COLOR, red, green, blue, 0, 0};
        led_task_send_command(&msg);
        last_red = red; last_green = green; last_blue = blue;
        first_run = false;
    } else if (red != last_red || green != last_green || blue != last_blue) {
        led_task_fade_to_color(red, green, blue);
        last_red = red; last_green = green; last_blue = blue;
    }
    
    if (displayPresent) {
        uint8 levelOled = (zclApp_Config.LightIndLevel == 0) ? 2 : map_value(zclApp_Config.LightIndLevel, 1, 127, 2, 254);
        ssd1306_SetContrast(levelOled);
    }
}

/*********************************************************************
 * @fn      zclApp_OnOffCB
 *
 * @brief   On/Off cluster command callback
 *********************************************************************/

static void zclApp_OnOffCB(uint8 cmd) {
    if (cmd == COMMAND_ON) {
        zclApp_Config.LightIndicator = 1;
        zclApp_rgbRun();
        
    } else if (cmd == COMMAND_OFF) {
        zclApp_Config.LightIndicator = 0;
        zclApp_rgbRun();
    }
}

/*********************************************************************
 * @fn      zclLevel_MoveToLevelCB
 *
 * @brief   Level Control Move to Level command callback
 *********************************************************************/

void zclLevel_MoveToLevelCB(zclLCMoveToLevel_t *pCmd) {
    if (pCmd->transitionTime == 0) {
        zclLevel_LevelRemainingTime = 1;
    } else {
        zclLevel_LevelRemainingTime = pCmd->transitionTime;
    }
    
    zclLevel_CurrentLevel = pCmd->level;
    zclApp_Config.LightIndLevel = zclLevel_CurrentLevel / 2;
    zclApp_SaveAttributesToNV();
    
    if (zclApp_Config.LightIndLevel > 0) {
        zclApp_OnOffCB(COMMAND_ON);
    } else if (zclApp_Config.LightIndLevel == 0) {
        zclApp_OnOffCB(COMMAND_OFF);
    }
}

/*********************************************************************
 * @fn      map_value
 *
 * @brief   Map value from one range to another
 *********************************************************************/

uint8_t map_value(uint8_t input, uint8_t in_min, uint8_t in_max, uint8_t out_min, uint8_t out_max) {
    if (in_min == in_max) {
        return out_min;
    }
    
    if (input < in_min) {
        input = in_min;
    } else if (input > in_max) {
        input = in_max;
    }
    
    int32_t result = (int32_t)(input - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    
    if (result < 0) {
        return 0;
    } else if (result > 255) {
        return 255;
    }
    
    return (uint8_t)result;
}

/*********************************************************************
 * @fn      zclApp_OnOff_Send
 *
 * @brief   Send On/Off attribute report
 *********************************************************************/

static void zclApp_OnOff_Send(void) {
    if (allRep == true) {
        const uint8 NUM_ATTRIBUTES = 1;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
        
        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;
            pReportCmd->attrList[0].attrID = ATTRID_ON_OFF;
            pReportCmd->attrList[0].dataType = ZCL_DATATYPE_BOOLEAN;
            pReportCmd->attrList[0].attrData = (void *)(&zclApp_Config.LightIndicator);
            
            zcl_SendReportCmd(1, &inderect_DstAddr, ZCL_CLUSTER_ID_GEN_ON_OFF, pReportCmd, ZCL_FRAME_SERVER_CLIENT_DIR, true, SeqNum++);
            osal_mem_free(pReportCmd);
        }
    } else {
        bdb_RepChangedAttrValue(1, ZCL_CLUSTER_ID_GEN_ON_OFF, ATTRID_ON_OFF);
    }
}

/*********************************************************************
 * @fn      zclApp_Level_Send
 *
 * @brief   Send Level attribute report
 *********************************************************************/

static void zclApp_Level_Send(void) {
    if (allRep == true) {
        const uint8 NUM_ATTRIBUTES = 1;
        zclReportCmd_t *pReportCmd;
        pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
        
        if (pReportCmd != NULL) {
            pReportCmd->numAttr = NUM_ATTRIBUTES;
            pReportCmd->attrList[0].attrID = ATTRID_LEVEL_CURRENT_LEVEL;
            pReportCmd->attrList[0].dataType = ZCL_DATATYPE_UINT8;
            pReportCmd->attrList[0].attrData = (void *)(&zclLevel_CurrentLevel);
            
            zcl_SendReportCmd(1, &inderect_DstAddr, ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL, pReportCmd, ZCL_FRAME_SERVER_CLIENT_DIR, true, SeqNum++);
            osal_mem_free(pReportCmd);
        }
    } else {
        bdb_RepChangedAttrValue(1, ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL, ATTRID_LEVEL_CURRENT_LEVEL);
    }
}

/*********************************************************************
 * @fn      zclApp_ProcessIdentifyTimeChange
 *
 * @brief   Handle identify time change events
 *********************************************************************/

static void zclApp_ProcessIdentifyTimeChange(uint8 endpoint) {
    (void)endpoint;
    
    if (initOut == true) {
        if (zclApp_IdentifyTime > 0) {
            if (identifyStart == 0) {
                identifyStart = 1;
                // Start identification mode
                led_task_identify_start(current_rgb_red, current_rgb_green, current_rgb_blue);
            }
        } else {
            if (identifyStart == 1) {
                allRep = true;
                identifyStart = 0;
                
                led_task_identify_stop();
                
                force_led_update = true;
                zclApp_rgbRun();
                zclApp_Report();
            }
        }
    }
}

#if defined (OTA_CLIENT) && (OTA_CLIENT == TRUE)
/*********************************************************************
 * @fn      zclApp_ProcessOTAMsgs
 *
 * @brief   Process OTA callbacks
 *********************************************************************/

static void zclApp_ProcessOTAMsgs(zclOTA_CallbackMsg_t* pMsg) {
    uint8 RxOnIdle;
    
    switch (pMsg->ota_event) {
    case ZCL_OTA_START_CALLBACK:
        if (pMsg->hdr.status == ZSuccess) {
            RxOnIdle = TRUE;
            ZMacSetReq(ZMacRxOnIdle, &RxOnIdle);
            NLME_SetPollRate(300);
        }
        break;
        
    case ZCL_OTA_DL_COMPLETE_CALLBACK:
        if (pMsg->hdr.status == ZSuccess) {
            HalOTAInvRC();
            SystemReset();
        }
#if (ZG_BUILD_ENDDEVICE_TYPE)
        else {
            RxOnIdle = FALSE;
            ZMacSetReq(ZMacRxOnIdle, &RxOnIdle);
            NLME_SetPollRate(DEVICE_POLL_RATE);
        }
#endif
        break;
        
    default:
        break;
    }
}
#endif

/*********************************************************************
 * OLED Display Functions (with detection support)
 *********************************************************************/

void oledGo(void) {
    if (!displayPresent) return;
    
    oledSetColor(zclApp_Config.invert);
    
    ssd1306_Init();
    ssd1306_SetContrast(0x00);
    ssd1306_Fill(currentBackgroundDisplayColor);
    ssd1306_DrawBitmap(0, 0, logo, 128, 64, currentDisplayColor);
    ssd1306_UpdateScreen();
    
    DelayMs(500);

    uint8 st;
    uint32 mss = 100;
    
    // Fade-in animation
    for (st = 1; st < 200; st++) {
        ssd1306_SetContrast(st);
        DelayMs(mss);
        mss--;
        mss--;
        mss--;
        mss--;
        mss--;
        if (mss == 10) {
            mss = 15;
        }
    }
}

void oledRefresh(void) {
    if (!displayPresent) return;
    
    // If ENS160 is not present, only show PM data
    if (!ens160Present) {
        oledPMdata((uint16)zclApp_Sensors.pm25);
        ssd1306_UpdateScreen();
        return;
    }
    
    // If ENS160 is present, cycle through display modes
    switch (oled_display_mode) {
        case 0:
            oledPMdata((uint16)zclApp_Sensors.pm25);
            break;
        case 1:
            oledTVOCdata((uint16)zclApp_Sensors.tvoc);
            break;
        case 2:
            oledCO2data((uint16)zclApp_Sensors.eco2);
            break;
    }
    
    oled_display_mode++;
    if (oled_display_mode >= 3) {
        oled_display_mode = 0;
    }
    
    ssd1306_UpdateScreen();
}

void oledPMdata(uint16 pmLevel) {
    if (!displayPresent) return;
    
    ssd1306_DrawBitmap(45, 6, b1, 38, 7, currentBackgroundDisplayColor);
    ssd1306_DrawBitmap(46, 49, b2, 38, 11, currentBackgroundDisplayColor);
    
    ssd1306_DrawBitmap(45, 6, pm25, 38, 7, currentDisplayColor);
    ssd1306_DrawBitmap(46, 49, mg3, 38, 11, currentDisplayColor);
    
    uint8 x = 20;
    uint8 y = 0;
    
    if (pmLevel >= 10000) {
        pmLevel = 9999;
    }
    
    // Clear previous digits based on number of digits
    if (lengthPM == 1) {
        y = 52;
        ssd1306_DrawBitmap(y, x, backL, 23, 23, currentBackgroundDisplayColor);
    } else if (lengthPM == 2) {
        y = 40;
        ssd1306_DrawBitmap(y, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 25, x, backL, 23, 23, currentBackgroundDisplayColor);
    } else if (lengthPM == 3) {
        y = 27;
        ssd1306_DrawBitmap(y, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 25, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 50, x, backL, 23, 23, currentBackgroundDisplayColor);
    } else if (lengthPM == 4) {
        y = 15;
        ssd1306_DrawBitmap(y, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 25, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 50, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 75, x, backL, 23, 23, currentBackgroundDisplayColor);
    }
    
    // Determine number of digits
    if (pmLevel < 10) {
        y = 52;
        lengthPM = 1;
    } else if (pmLevel >= 10 && pmLevel < 100) {
        lengthPM = 2;
        y = 40;
    } else if (pmLevel >= 100 && pmLevel < 1000) {
        lengthPM = 3;
        y = 27;
    } else if (pmLevel >= 1000 && pmLevel <= 10000) {
        lengthPM = 4;
        y = 15;
    }
    
    // Extract digits
    byte one_pm = pmLevel / 1000;
    byte two_pm = pmLevel % 1000 / 100;
    byte three_pm = pmLevel % 100 / 10;
    byte fouth_pm = pmLevel % 10;
    
    // 1-digit display
    if (lengthPM == 1) {
        switch (fouth_pm) {
            case 0: ssd1306_DrawBitmap(y, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y, x, nineL, 23, 23, currentDisplayColor); break;
        }
    }
    
    // 2-digit display
    if (lengthPM == 2) {
        switch (three_pm) {
            case 0: ssd1306_DrawBitmap(y, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (fouth_pm) {
            case 0: ssd1306_DrawBitmap(y + 25, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 25, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 25, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 25, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 25, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 25, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 25, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 25, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 25, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 25, x, nineL, 23, 23, currentDisplayColor); break;
        }
    }
    
    // 3-digit display
    if (lengthPM == 3) {
        switch (two_pm) {
            case 0: ssd1306_DrawBitmap(y, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (three_pm) {
            case 0: ssd1306_DrawBitmap(y + 25, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 25, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 25, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 25, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 25, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 25, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 25, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 25, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 25, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 25, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (fouth_pm) {
            case 0: ssd1306_DrawBitmap(y + 50, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 50, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 50, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 50, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 50, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 50, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 50, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 50, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 50, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 50, x, nineL, 23, 23, currentDisplayColor); break;
        }
    }
    
    // 4-digit display
    if (lengthPM == 4) {
        switch (one_pm) {
            case 0: ssd1306_DrawBitmap(y, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (two_pm) {
            case 0: ssd1306_DrawBitmap(y + 25, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 25, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 25, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 25, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 25, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 25, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 25, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 25, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 25, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 25, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (three_pm) {
            case 0: ssd1306_DrawBitmap(y + 50, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 50, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 50, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 50, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 50, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 50, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 50, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 50, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 50, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 50, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (fouth_pm) {
            case 0: ssd1306_DrawBitmap(y + 75, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 75, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 75, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 75, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 75, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 75, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 75, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 75, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 75, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 75, x, nineL, 23, 23, currentDisplayColor); break;
        }
    }
}

void oledTVOCdata(uint16 tvocLevel) {
    if (!displayPresent) return;
    if (!ens160Present) return;
    
    ssd1306_DrawBitmap(45, 6, b1, 38, 7, currentBackgroundDisplayColor);
    ssd1306_DrawBitmap(46, 49, b2, 38, 11, currentBackgroundDisplayColor);
    
    ssd1306_DrawBitmap(48, 6, tvoc, 31, 7, currentDisplayColor);
    ssd1306_DrawBitmap(53, 51, ppb, 24, 7, currentDisplayColor);
    
    uint8 x = 20;
    uint8 y = 0;
    
    if (tvocLevel >= 10000) {
        tvocLevel = 9999;
    }
    
    // Clear previous digits based on number of digits
    if (lengthPM == 1) {
        y = 52;
        ssd1306_DrawBitmap(y, x, backL, 23, 23, currentBackgroundDisplayColor);
    } else if (lengthPM == 2) {
        y = 40;
        ssd1306_DrawBitmap(y, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 25, x, backL, 23, 23, currentBackgroundDisplayColor);
    } else if (lengthPM == 3) {
        y = 27;
        ssd1306_DrawBitmap(y, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 25, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 50, x, backL, 23, 23, currentBackgroundDisplayColor);
    } else if (lengthPM == 4) {
        y = 15;
        ssd1306_DrawBitmap(y, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 25, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 50, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 75, x, backL, 23, 23, currentBackgroundDisplayColor);
    }
    
    // Determine number of digits
    if (tvocLevel < 10) {
        y = 52;
        lengthPM = 1;
    } else if (tvocLevel >= 10 && tvocLevel < 100) {
        lengthPM = 2;
        y = 40;
    } else if (tvocLevel >= 100 && tvocLevel < 1000) {
        lengthPM = 3;
        y = 27;
    } else if (tvocLevel >= 1000 && tvocLevel <= 10000) {
        lengthPM = 4;
        y = 15;
    }
    
    // Extract digits
    byte one_pm = tvocLevel / 1000;
    byte two_pm = tvocLevel % 1000 / 100;
    byte three_pm = tvocLevel % 100 / 10;
    byte fouth_pm = tvocLevel % 10;
    
    // 1-digit display
    if (lengthPM == 1) {
        switch (fouth_pm) {
            case 0: ssd1306_DrawBitmap(y, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y, x, nineL, 23, 23, currentDisplayColor); break;
        }
    }
    
    // 2-digit display
    if (lengthPM == 2) {
        switch (three_pm) {
            case 0: ssd1306_DrawBitmap(y, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (fouth_pm) {
            case 0: ssd1306_DrawBitmap(y + 25, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 25, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 25, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 25, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 25, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 25, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 25, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 25, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 25, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 25, x, nineL, 23, 23, currentDisplayColor); break;
        }
    }
    
    // 3-digit display
    if (lengthPM == 3) {
        switch (two_pm) {
            case 0: ssd1306_DrawBitmap(y, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (three_pm) {
            case 0: ssd1306_DrawBitmap(y + 25, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 25, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 25, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 25, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 25, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 25, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 25, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 25, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 25, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 25, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (fouth_pm) {
            case 0: ssd1306_DrawBitmap(y + 50, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 50, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 50, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 50, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 50, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 50, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 50, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 50, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 50, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 50, x, nineL, 23, 23, currentDisplayColor); break;
        }
    }
    
    // 4-digit display
    if (lengthPM == 4) {
        switch (one_pm) {
            case 0: ssd1306_DrawBitmap(y, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (two_pm) {
            case 0: ssd1306_DrawBitmap(y + 25, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 25, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 25, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 25, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 25, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 25, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 25, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 25, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 25, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 25, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (three_pm) {
            case 0: ssd1306_DrawBitmap(y + 50, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 50, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 50, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 50, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 50, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 50, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 50, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 50, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 50, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 50, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (fouth_pm) {
            case 0: ssd1306_DrawBitmap(y + 75, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 75, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 75, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 75, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 75, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 75, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 75, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 75, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 75, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 75, x, nineL, 23, 23, currentDisplayColor); break;
        }
    }
}

void oledCO2data(uint16 co2Level) {
    if (!displayPresent) return;
    if (!ens160Present) return;
    
    ssd1306_DrawBitmap(45, 6, b1, 38, 7, currentBackgroundDisplayColor);
    ssd1306_DrawBitmap(46, 49, b2, 38, 11, currentBackgroundDisplayColor);
    
    ssd1306_DrawBitmap(49, 6, eco2, 29, 7, currentDisplayColor);
    ssd1306_DrawBitmap(53, 51, ppm, 25, 7, currentDisplayColor);
    
    uint8 x = 20;
    uint8 y = 0;
    
    if (co2Level >= 10000) {
        co2Level = 9999;
    }
    
    // Clear previous digits based on number of digits
    if (lengthPM == 1) {
        y = 52;
        ssd1306_DrawBitmap(y, x, backL, 23, 23, currentBackgroundDisplayColor);
    } else if (lengthPM == 2) {
        y = 40;
        ssd1306_DrawBitmap(y, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 25, x, backL, 23, 23, currentBackgroundDisplayColor);
    } else if (lengthPM == 3) {
        y = 27;
        ssd1306_DrawBitmap(y, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 25, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 50, x, backL, 23, 23, currentBackgroundDisplayColor);
    } else if (lengthPM == 4) {
        y = 15;
        ssd1306_DrawBitmap(y, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 25, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 50, x, backL, 23, 23, currentBackgroundDisplayColor);
        ssd1306_DrawBitmap(y + 75, x, backL, 23, 23, currentBackgroundDisplayColor);
    }
    
    // Determine number of digits
    if (co2Level < 10) {
        y = 52;
        lengthPM = 1;
    } else if (co2Level >= 10 && co2Level < 100) {
        lengthPM = 2;
        y = 40;
    } else if (co2Level >= 100 && co2Level < 1000) {
        lengthPM = 3;
        y = 27;
    } else if (co2Level >= 1000 && co2Level <= 10000) {
        lengthPM = 4;
        y = 15;
    }
    
    // Extract digits
    byte one_pm = co2Level / 1000;
    byte two_pm = co2Level % 1000 / 100;
    byte three_pm = co2Level % 100 / 10;
    byte fouth_pm = co2Level % 10;
    
    // 1-digit display
    if (lengthPM == 1) {
        switch (fouth_pm) {
            case 0: ssd1306_DrawBitmap(y, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y, x, nineL, 23, 23, currentDisplayColor); break;
        }
    }
    
    // 2-digit display
    if (lengthPM == 2) {
        switch (three_pm) {
            case 0: ssd1306_DrawBitmap(y, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (fouth_pm) {
            case 0: ssd1306_DrawBitmap(y + 25, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 25, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 25, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 25, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 25, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 25, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 25, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 25, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 25, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 25, x, nineL, 23, 23, currentDisplayColor); break;
        }
    }
    
    // 3-digit display
    if (lengthPM == 3) {
        switch (two_pm) {
            case 0: ssd1306_DrawBitmap(y, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (three_pm) {
            case 0: ssd1306_DrawBitmap(y + 25, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 25, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 25, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 25, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 25, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 25, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 25, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 25, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 25, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 25, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (fouth_pm) {
            case 0: ssd1306_DrawBitmap(y + 50, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 50, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 50, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 50, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 50, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 50, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 50, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 50, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 50, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 50, x, nineL, 23, 23, currentDisplayColor); break;
        }
    }
    
    // 4-digit display
    if (lengthPM == 4) {
        switch (one_pm) {
            case 0: ssd1306_DrawBitmap(y, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (two_pm) {
            case 0: ssd1306_DrawBitmap(y + 25, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 25, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 25, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 25, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 25, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 25, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 25, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 25, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 25, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 25, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (three_pm) {
            case 0: ssd1306_DrawBitmap(y + 50, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 50, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 50, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 50, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 50, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 50, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 50, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 50, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 50, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 50, x, nineL, 23, 23, currentDisplayColor); break;
        }
        switch (fouth_pm) {
            case 0: ssd1306_DrawBitmap(y + 75, x, zeroL, 23, 23, currentDisplayColor); break;
            case 1: ssd1306_DrawBitmap(y + 75, x, oneL, 23, 23, currentDisplayColor); break;
            case 2: ssd1306_DrawBitmap(y + 75, x, twoL, 23, 23, currentDisplayColor); break;
            case 3: ssd1306_DrawBitmap(y + 75, x, threeL, 23, 23, currentDisplayColor); break;
            case 4: ssd1306_DrawBitmap(y + 75, x, fourL, 23, 23, currentDisplayColor); break;
            case 5: ssd1306_DrawBitmap(y + 75, x, fiveL, 23, 23, currentDisplayColor); break;
            case 6: ssd1306_DrawBitmap(y + 75, x, sixL, 23, 23, currentDisplayColor); break;
            case 7: ssd1306_DrawBitmap(y + 75, x, sevenL, 23, 23, currentDisplayColor); break;
            case 8: ssd1306_DrawBitmap(y + 75, x, eightL, 23, 23, currentDisplayColor); break;
            case 9: ssd1306_DrawBitmap(y + 75, x, nineL, 23, 23, currentDisplayColor); break;
        }
    }
}

void oledSetColor(bool invert) {
    if (invert) {
        currentDisplayColor = Black;
        currentBackgroundDisplayColor = White;
    } else {
        currentDisplayColor = White;
        currentBackgroundDisplayColor = Black;
    }
}

void readVOC(void) {
    // If no sensor - skip
    if (!ens160Present) {
        return;
    }
    
    uint16_t aqi, tvoc, eco2;
    uint8_t validity;
    bool ready;
    
    // Check data ready flag (NEWDAT = 1)
    if (ens160_is_data_ready(&ready) == ENS160_SUCCESS && ready) {
        // Read AQI with validity
        if (ens160_read_aqi(&aqi, &validity) == ENS160_SUCCESS) {
            zclApp_Sensors.aqi = (float)aqi;
        }
        
        // Read TVOC
        if (ens160_read_tvoc(&tvoc) == ENS160_SUCCESS) {
            zclApp_Sensors.tvoc = (float)tvoc;
        }
        
        // Read eCO2
        if (ens160_read_eco2(&eco2) == ENS160_SUCCESS) {
            zclApp_Sensors.eco2 = (float)eco2;
        }
    }
}

/*********************************************************************
 * @fn      ens160_Detect
 * @brief   Detect if ENS160 sensor is connected by checking PART_ID
 * @return  true if sensor detected, false otherwise
 *********************************************************************/
static bool ens160_Detect(void) {
    uint16_t part_id = 0;
    uint8_t rev_id = 0;
    
    // Read PART_ID register
    if (ens160_get_version(&part_id, &rev_id) == ENS160_SUCCESS) {
        if (part_id == ENS160_PART_ID_VAL) {
            return true;  // Sensor found
        }
    }
    return false;  // Sensor not found
}
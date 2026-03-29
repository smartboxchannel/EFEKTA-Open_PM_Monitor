#include "factory_reset.h"
#include "AF.h"
#include "OnBoard.h"
#include "bdb.h"
#include "bdb_interface.h"
#include "ZComDef.h"
#include "hal_key.h"
#include "zcl_app.h"
#include "ZDApp.h"

/******************************************************************************
 * External Variables
 *****************************************************************************/

extern devStates_t devState;

/******************************************************************************
 * Local Function Prototypes
 *****************************************************************************/

static void zclFactoryResetter_ResetToFN(void);
static void zclFactoryResetter_ProcessBootCounter(void);
static void zclFactoryResetter_ResetBootCounter(void);
static void zclFactoryResetter_LedOn(void);
static void zclFactoryResetter_LedOff(void);

/******************************************************************************
 * Global Variables
 *****************************************************************************/

static uint8 zclFactoryResetter_TaskID;

/******************************************************************************
 * @fn      zclFactoryResetter_loop
 *
 * @brief   Factory reset task event loop
 *****************************************************************************/

uint16 zclFactoryResetter_loop(uint8 task_id, uint16 events)
{
    if (events & FACTORY_RESET_EVT)
    {
        zclFactoryResetter_ResetToFN();
        return (events ^ FACTORY_RESET_EVT);
    }

    if (events & FACTORY_BOOTCOUNTER_RESET_EVT)
    {
        zclFactoryResetter_ResetBootCounter();
        return (events ^ FACTORY_BOOTCOUNTER_RESET_EVT);
    }

    if (events & FACTORY_LED_EVT)
    {
        zclFactoryResetter_LedOn();
        return (events ^ FACTORY_LED_EVT);
    }

    if (events & FACTORY_LEDOFF_EVT)
    {
        zclFactoryResetter_LedOff();
        return (events ^ FACTORY_LEDOFF_EVT);
    }

    return 0;
}

/******************************************************************************
 * @fn      zclFactoryResetter_ResetBootCounter
 *
 * @brief   Clear boot counter in NV
 *****************************************************************************/

void zclFactoryResetter_ResetBootCounter(void)
{
    uint16 bootCnt = 0;
    osal_nv_write(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
}

/******************************************************************************
 * @fn      zclFactoryResetter_Init
 *
 * @brief   Initialize factory reset module
 *****************************************************************************/

void zclFactoryResetter_Init(uint8 task_id)
{
    zclFactoryResetter_TaskID = task_id;

#if FACTORY_RESET_BY_BOOT_COUNTER
    zclFactoryResetter_ProcessBootCounter();
#endif
}

/******************************************************************************
 * @fn      zclFactoryResetter_ResetToFN
 *
 * @brief   Perform factory reset to factory new state
 *****************************************************************************/

static void zclFactoryResetter_ResetToFN(void)
{
    HAL_TURN_ON_LED1();

    osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT);
    osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LEDOFF_EVT);

    bdb_resetLocalAction();
}

/******************************************************************************
 * @fn      zclFactoryResetter_HandleKeys
 *
 * @brief   Handle key presses for factory reset
 *****************************************************************************/

void zclFactoryResetter_HandleKeys(uint8 portAndAction, uint8 keyCode)
{
#if FACTORY_RESET_BY_LONG_PRESS
    // Factory reset only when device is already on network
    if (devState != DEV_ROUTER &&
        devState != DEV_END_DEVICE &&
        devState != DEV_END_DEVICE_UNAUTH)
    {
        // Device not on network - ignore factory reset
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT);
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT);
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LEDOFF_EVT);
        HAL_TURN_OFF_LED1();
        return;
    }

    if (portAndAction & HAL_KEY_RELEASE)
    {
        // Key released - cancel pending reset
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT);
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT);
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_LEDOFF_EVT);
        HAL_TURN_OFF_LED1();
    }
    else
    {
        // Key pressed - start long press timer
        bool isCorrectPort = true;
#if FACTORY_RESET_BY_LONG_PRESS_PORT
        isCorrectPort = (FACTORY_RESET_BY_LONG_PRESS_PORT & portAndAction) != 0;
#endif

        if (isCorrectPort)
        {
            uint32 timeout = FACTORY_RESET_HOLD_TIME_LONG;

            osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT, timeout);
            osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT, 2000);
        }
    }
#endif
}

/******************************************************************************
 * @fn      zclFactoryResetter_ProcessBootCounter
 *
 * @brief   Check boot counter for auto factory reset
 *****************************************************************************/

static void zclFactoryResetter_ProcessBootCounter(void)
{
    osal_start_timerEx(zclFactoryResetter_TaskID,
                       FACTORY_BOOTCOUNTER_RESET_EVT,
                       FACTORY_RESET_BOOTCOUNTER_RESET_TIME);

    uint16 bootCnt = 0;

    if (osal_nv_item_init(ZCD_NV_BOOTCOUNTER, sizeof(bootCnt), &bootCnt) == ZSUCCESS)
    {
        osal_nv_read(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
    }

    bootCnt++;

    if (bootCnt >= FACTORY_RESET_BOOTCOUNTER_MAX_VALUE)
    {
        bootCnt = 0;
        osal_stop_timerEx(zclFactoryResetter_TaskID, FACTORY_BOOTCOUNTER_RESET_EVT);
        osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_RESET_EVT, 5000);
    }

    osal_nv_write(ZCD_NV_BOOTCOUNTER, 0, sizeof(bootCnt), &bootCnt);
}

/******************************************************************************
 * @fn      zclFactoryResetter_LedOn
 *
 * @brief   Turn LED on with blink pattern
 *****************************************************************************/

static void zclFactoryResetter_LedOn(void)
{
    HAL_TURN_ON_LED1();
    osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_LEDOFF_EVT, 50);
}

/******************************************************************************
 * @fn      zclFactoryResetter_LedOff
 *
 * @brief   Turn LED off with blink pattern
 *****************************************************************************/

static void zclFactoryResetter_LedOff(void)
{
    HAL_TURN_OFF_LED1();
    osal_start_timerEx(zclFactoryResetter_TaskID, FACTORY_LED_EVT, 1000);
}
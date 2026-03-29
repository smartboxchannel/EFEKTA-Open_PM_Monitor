/******************************************************************************
 * Filename:       OSAL_App.c
 * Description:    OSAL Task Initialization for Application
 *****************************************************************************/

#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "ZComDef.h"
#include "hal_drivers.h"

#include "APS.h"
#include "ZDApp.h"
#include "nwk.h"

#include "bdb_interface.h"
#include "zcl_app.h"
#include "factory_reset.h"
#include "commissioning.h"
#include "pwm_rgb.h"
#include "led_task.h"

#if defined (MT_TASK)
    #include "MT.h"
    #include "MT_TASK.h"
#endif

#if (defined OTA_CLIENT) && (OTA_CLIENT == TRUE)
    #include "zcl_ota.h"
#endif

/******************************************************************************
 * Task Event Handler Table
 *****************************************************************************/

const pTaskEventHandlerFn tasksArr[] = {
    macEventLoop,
    nwk_event_loop,
    Hal_ProcessEvent,
#if defined(MT_TASK)
    MT_ProcessEvent,
#endif
    APS_event_loop,
    ZDApp_event_loop,
    pwm_rgb_event_loop,
    led_task_event_loop,
    zcl_event_loop,
    bdb_event_loop,
    zclApp_event_loop,
    zclFactoryResetter_loop,
    zclCommissioning_event_loop,
#if (defined OTA_CLIENT) && (OTA_CLIENT == TRUE)
    zclOTA_event_loop
#endif
};

const uint8 tasksCnt = sizeof(tasksArr) / sizeof(tasksArr[0]);
uint16 *tasksEvents;

/******************************************************************************
 * @fn      osalInitTasks
 *
 * @brief   Initialize all OSAL tasks
 *****************************************************************************/

void osalInitTasks(void)
{
    uint8 taskID = 0;

    // Allocate memory for task events
    tasksEvents = (uint16 *)osal_mem_alloc(sizeof(uint16) * tasksCnt);
    osal_memset(tasksEvents, 0, (sizeof(uint16) * tasksCnt));

    // Initialize all tasks in order
    macTaskInit(taskID++);
    nwk_init(taskID++);
    Hal_Init(taskID++);

#if defined(MT_TASK)
    MT_TaskInit(taskID++);
#endif

    APS_Init(taskID++);
    ZDApp_Init(taskID++);
    pwm_rgb_Init(taskID++);
    led_task_Init(taskID++);
    zcl_Init(taskID++);
    bdb_Init(taskID++);
    zclApp_Init(taskID++);
    zclFactoryResetter_Init(taskID++);
    zclCommissioning_Init(taskID++);

#if (defined OTA_CLIENT) && (OTA_CLIENT == TRUE)
    zclOTA_Init(taskID++);
#endif
}

/******************************************************************************
 * END OF FILE
 *****************************************************************************/
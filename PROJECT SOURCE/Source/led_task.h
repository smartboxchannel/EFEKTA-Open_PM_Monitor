#ifndef LED_TASK_H
#define LED_TASK_H

#include "ZComDef.h"

/*********************************************************************
 * LED Task Event Definitions
 *********************************************************************/

#define LED_TASK_ARROW_EVT       0x0002   // Arrow animation event
#define LED_TASK_FADE_EVT        0x0004   // Color fade animation event
#define LED_TASK_IDENTIFY_EVT    0x0008   // Identify mode animation event

/*********************************************************************
 * LED Command Definitions
 *********************************************************************/

#define LED_CMD_NONE      0              // No command
#define LED_CMD_SET_COLOR 1              // Set color immediately

/*********************************************************************
 * LED Message Structure
 *********************************************************************/

typedef struct {
    uint8 cmd;      // Command type (LED_CMD_*)
    uint8 red;      // Red component (0-255)
    uint8 green;    // Green component (0-255)
    uint8 blue;     // Blue component (0-255)
    uint8 steps;    // Number of fade steps (unused)
    uint16 delay;   // Step delay in ms (unused)
} led_message_t;

/*********************************************************************
 * Function Prototypes
 *********************************************************************/

/*
 * Initialize LED task
 * @param task_id - Task ID for event handling
 */
extern void led_task_Init(byte task_id);

/*
 * LED task event loop
 * @param task_id - Task ID
 * @param events - Events to process
 * @return Unprocessed events
 */
extern uint16 led_task_event_loop(uint8 task_id, uint16 events);

/*
 * Send command to LED task
 * @param msg - Pointer to LED message structure
 */
extern void led_task_send_command(led_message_t *msg);

/*
 * Send arrow indicator value with smooth animation
 * @param value - Target arrow value (0-255)
 */
extern void led_task_send_arrow(uint8 value);

/*
 * Set arrow indicator value immediately (without animation)
 * @param value - Arrow value (0-255)
 */
extern void led_task_set_arrow_immediate(uint8 value);

/*
 * Fade RGB color from current to target
 * @param red - Target red component (0-255)
 * @param green - Target green component (0-255)
 * @param blue - Target blue component (0-255)
 */
extern void led_task_fade_to_color(uint8 red, uint8 green, uint8 blue);

/*
 * Set initial arrow position
 * @param value - Initial arrow value (0-255)
 */
extern void led_task_set_arrow_start(uint8 value);

/*
 * Start identification mode (pulsing blue light)
 * @param red - Red component to restore after identification
 * @param green - Green component to restore after identification
 * @param blue - Blue component to restore after identification
 */
extern void led_task_identify_start(uint8 red, uint8 green, uint8 blue);

/*
 * Stop identification mode and restore previous color
 */
extern void led_task_identify_stop(void);

#endif /* LED_TASK_H */
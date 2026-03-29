#ifndef PWM_RGB_H
#define PWM_RGB_H

#include "OSAL.h"
#include "ZComDef.h"

/*********************************************************************
 * PWM Channel Definitions
 *********************************************************************/

#define PWM_CHANNEL_RED             4
#define PWM_CHANNEL_GREEN           3
#define PWM_CHANNEL_BLUE            2
#define RETRO_VOLTMETER_CHANNEL     1

#define PWM_DEFAULT_FREQ            125000

/*********************************************************************
 * Function Prototypes
 *********************************************************************/

/*
 * Initialize PWM RGB module
 * @param task_id - Task ID for event handling
 */
extern void pwm_rgb_Init(byte task_id);

/*
 * Event loop for PWM RGB module
 * @param task_id - Task ID
 * @param events - Events to process
 * @return Unprocessed events
 */
extern uint16 pwm_rgb_event_loop(uint8 task_id, uint16 events);

/*
 * Set RGB color
 * @param red - Red value (0-255)
 * @param green - Green value (0-255)
 * @param blue - Blue value (0-255)
 */
extern void pwm_rgb_set_color(uint8 red, uint8 green, uint8 blue);

/*
 * Set single PWM channel duty cycle
 * @param channel - PWM channel number (1-4)
 * @param duty - Duty cycle value (0-255)
 */
extern void pwm_rgb_set_channel(uint8 channel, uint8 duty);

#endif /* PWM_RGB_H */
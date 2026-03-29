#include "led_task.h"
#include "pwm_rgb.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

/*********************************************************************
 * Constants
 *********************************************************************/

#define FADE_TOTAL_STEPS     30   // Number of steps for color fade
#define FADE_STEP_DELAY      30   // Delay between fade steps (ms)
#define ARROW_TOTAL_STEPS    40   // Number of steps for arrow animation
#define ARROW_STEP_DELAY     50   // Delay between arrow steps (ms)
#define IDENTIFY_STEP_DELAY  10   // Delay between identification steps (ms)
#define IDENTIFY_STEP_SIZE   2    // Step size for identification brightness

/*********************************************************************
 * Local Variables
 *********************************************************************/

static byte led_task_id = 0xFF;

// RGB color state
static uint8 current_red = 0;
static uint8 current_green = 0;
static uint8 current_blue = 0;

// Fade animation state
static uint8 target_red = 0;
static uint8 target_green = 0;
static uint8 target_blue = 0;
static uint8 fade_step = 0;
static uint8 fade_phase = 0;           // 0: idle, 1: fade to max, 2: fade to target
static bool fade_active = false;
static uint8 pending_red = 0;
static uint8 pending_green = 0;
static uint8 pending_blue = 0;

// Arrow indicator state
static uint8 current_arrow = 128;
static uint8 target_arrow = 128;
static uint8 arrow_step = 0;
static bool arrow_animation_active = false;
static uint8 arrow_start_value = 128;

// Identification mode state
static bool identify_active = false;
static uint8 identify_step = 0;
static uint8 identify_direction = 0;   // 0: increasing, 1: decreasing
static uint8 saved_red = 0;
static uint8 saved_green = 0;
static uint8 saved_blue = 0;

/*********************************************************************
 * Local Function Prototypes
 *********************************************************************/

static void apply_arrow(uint8 value);
static void apply_color(uint8 red, uint8 green, uint8 blue);
static void identify_cycle(void);

/*********************************************************************
 * @fn      led_task_set_arrow_start
 *
 * @brief   Set initial arrow position
 *********************************************************************/

void led_task_set_arrow_start(uint8 value) {
    arrow_start_value = value;
    current_arrow = value;
}

/*********************************************************************
 * @fn      led_task_Init
 *
 * @brief   Initialize LED task
 *********************************************************************/

void led_task_Init(byte task_id) {
    led_task_id = task_id;
    current_arrow = arrow_start_value;
    pwm_rgb_set_channel(RETRO_VOLTMETER_CHANNEL, current_arrow);
    osal_pwrmgr_task_state(led_task_id, PWRMGR_CONSERVE);
}

/*********************************************************************
 * @fn      led_task_send_command
 *
 * @brief   Send command to LED task (immediate color set)
 *********************************************************************/

void led_task_send_command(led_message_t *msg) {
    if (msg->cmd == LED_CMD_SET_COLOR) {
        pwm_rgb_set_color(msg->red, msg->green, msg->blue);
        current_red = msg->red;
        current_green = msg->green;
        current_blue = msg->blue;
        fade_active = false;
        fade_phase = 0;
    }
}

/*********************************************************************
 * @fn      led_task_fade_to_color
 *
 * @brief   Fade RGB color from current to target with two-phase animation
 *         Phase 1: Fade to full brightness (127)
 *         Phase 2: Fade to target color
 *********************************************************************/

void led_task_fade_to_color(uint8 red, uint8 green, uint8 blue) {
    if (fade_active) return;
    
    pending_red = red;
    pending_green = green;
    pending_blue = blue;
    
    // First fade to full brightness
    target_red = (current_red == 0) ? 0 : 127;
    target_green = (current_green == 0) ? 0 : 127;
    target_blue = (current_blue == 0) ? 0 : 127;
    
    fade_step = 0;
    fade_phase = 1;
    fade_active = true;
    osal_start_timerEx(led_task_id, LED_TASK_FADE_EVT, FADE_STEP_DELAY);
}

/*********************************************************************
 * @fn      led_task_send_arrow
 *
 * @brief   Send arrow indicator value with smooth animation
 *********************************************************************/

void led_task_send_arrow(uint8 value) {
    if (arrow_animation_active) return;
    if (current_arrow == value) return;
    
    target_arrow = value;
    arrow_step = 0;
    arrow_animation_active = true;
    osal_start_timerEx(led_task_id, LED_TASK_ARROW_EVT, ARROW_STEP_DELAY);
}

/*********************************************************************
 * @fn      led_task_set_arrow_immediate
 *
 * @brief   Set arrow indicator value immediately (without animation)
 *********************************************************************/

void led_task_set_arrow_immediate(uint8 value) {
    // Stop current animation
    arrow_animation_active = false;
    // Set new position immediately
    apply_arrow(value);
    target_arrow = value;
}

/*********************************************************************
 * @fn      led_task_identify_start
 *
 * @brief   Start identification mode - pulsating blue light
 *********************************************************************/

void led_task_identify_start(uint8 red, uint8 green, uint8 blue) {
    if (identify_active) return;
    
    // Save current color
    saved_red = red;
    saved_green = green;
    saved_blue = blue;
    
    identify_active = true;
    identify_step = 1;
    identify_direction = 0;  // Start with increasing brightness
    
    // Stop other animations
    fade_active = false;
    arrow_animation_active = false;
    
    // Start identification animation
    osal_start_timerEx(led_task_id, LED_TASK_IDENTIFY_EVT, IDENTIFY_STEP_DELAY);
}

/*********************************************************************
 * @fn      led_task_identify_stop
 *
 * @brief   Stop identification mode and restore previous color
 *********************************************************************/

void led_task_identify_stop(void) {
    if (!identify_active) return;
    
    identify_active = false;
    osal_stop_timerEx(led_task_id, LED_TASK_IDENTIFY_EVT);
    
    // Restore saved color
    pwm_rgb_set_color(saved_red, saved_green, saved_blue);
    current_red = saved_red;
    current_green = saved_green;
    current_blue = saved_blue;
}

/*********************************************************************
 * @fn      apply_arrow
 *
 * @brief   Apply arrow value to hardware
 *********************************************************************/

static void apply_arrow(uint8 value) {
    pwm_rgb_set_channel(RETRO_VOLTMETER_CHANNEL, value);
    current_arrow = value;
}

/*********************************************************************
 * @fn      apply_color
 *
 * @brief   Apply RGB color to hardware
 *********************************************************************/

static void apply_color(uint8 red, uint8 green, uint8 blue) {
    pwm_rgb_set_color(red, green, blue);
    current_red = red;
    current_green = green;
    current_blue = blue;
}

/*********************************************************************
 * @fn      identify_cycle
 *
 * @brief   Perform one step of identification animation (pulsing blue)
 *********************************************************************/

static void identify_cycle(void) {
    if (!identify_active) return;
    
    // Smooth brightness change
    if (identify_direction == 0) {
        identify_step += IDENTIFY_STEP_SIZE;
        if (identify_step >= 127) {
            identify_step = 127;
            identify_direction = 1;
        }
    } else {
        identify_step -= IDENTIFY_STEP_SIZE;
        if (identify_step <= 1) {
            identify_step = 1;
            identify_direction = 0;
        }
    }
    
    // Set blue color with current brightness
    pwm_rgb_set_color(0, 0, identify_step);
    current_red = 0;
    current_green = 0;
    current_blue = identify_step;
    
    // Schedule next step
    osal_start_timerEx(led_task_id, LED_TASK_IDENTIFY_EVT, IDENTIFY_STEP_DELAY);
}

/*********************************************************************
 * @fn      led_task_event_loop
 *
 * @brief   LED task event processing loop
 *********************************************************************/

uint16 led_task_event_loop(uint8 task_id, uint16 events) {
    // Identification mode has highest priority
    if (events & LED_TASK_IDENTIFY_EVT) {
        identify_cycle();
        return (events ^ LED_TASK_IDENTIFY_EVT);
    }
    
    // Color fade animation
    if (events & LED_TASK_FADE_EVT) {
        if (fade_active && !identify_active) {
            fade_step++;
            
            if (fade_step <= FADE_TOTAL_STEPS) {
                uint8 red = current_red + ((target_red - current_red) * fade_step / FADE_TOTAL_STEPS);
                uint8 green = current_green + ((target_green - current_green) * fade_step / FADE_TOTAL_STEPS);
                uint8 blue = current_blue + ((target_blue - current_blue) * fade_step / FADE_TOTAL_STEPS);
                pwm_rgb_set_color(red, green, blue);
                osal_start_timerEx(led_task_id, LED_TASK_FADE_EVT, FADE_STEP_DELAY);
            } else {
                if (fade_phase == 1) {
                    // Phase 1 complete - set to full brightness
                    uint8 new_red = (pending_red == 0) ? 0 : 127;
                    uint8 new_green = (pending_green == 0) ? 0 : 127;
                    uint8 new_blue = (pending_blue == 0) ? 0 : 127;
                    apply_color(new_red, new_green, new_blue);
                    
                    // Start phase 2 - fade to target color
                    target_red = pending_red;
                    target_green = pending_green;
                    target_blue = pending_blue;
                    fade_step = 0;
                    fade_phase = 2;
                    osal_start_timerEx(led_task_id, LED_TASK_FADE_EVT, FADE_STEP_DELAY);
                } else if (fade_phase == 2) {
                    // Phase 2 complete
                    apply_color(pending_red, pending_green, pending_blue);
                    fade_active = false;
                    fade_phase = 0;
                }
            }
        }
        return (events ^ LED_TASK_FADE_EVT);
    }
    
    // Arrow animation
    if (events & LED_TASK_ARROW_EVT) {
        if (arrow_animation_active && !identify_active) {
            arrow_step++;
            
            if (arrow_step <= ARROW_TOTAL_STEPS) {
                uint8 value = current_arrow + ((target_arrow - current_arrow) * arrow_step / ARROW_TOTAL_STEPS);
                pwm_rgb_set_channel(RETRO_VOLTMETER_CHANNEL, value);
                osal_start_timerEx(led_task_id, LED_TASK_ARROW_EVT, ARROW_STEP_DELAY);
            } else {
                apply_arrow(target_arrow);
                arrow_animation_active = false;
            }
        }
        return (events ^ LED_TASK_ARROW_EVT);
    }
    
    return 0;
}
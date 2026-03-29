#include "pwm_rgb.h"
#include "hal_types.h"
#include "hal_defs.h"
#include "hal_mcu.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

/*********************************************************************
 * Local Function Prototypes
 *********************************************************************/

static void pwm5_initTimer1(void);
void PWM5(uint32 freq, uint16 tduty, uint8 channal);

/*********************************************************************
 * Local Variables
 *********************************************************************/

static byte pwm_rgb_task_id = 0xFF;

/*********************************************************************
 * @fn      pwm5_initTimer1
 *
 * @brief   Initialize Timer1 for PWM output
 *********************************************************************/

static void pwm5_initTimer1(void) {
    // Configure P0.3, P0.4, P0.5, P0.6 as output
    P0DIR |= 0x78;
    
    // Select alternative 1 for Timer 1
    PERCFG &= ~0x40;
    
    // Select USART0, USART1 alternative 2
    PERCFG |= 0x03;
    
    // Timer 1 channels all have priority
    P2DIR &= ~0xC0;
    
    // Configure P0.3, P0.4, P0.5, P0.6 as peripheral
    P0SEL |= 0x78;
}

/*********************************************************************
 * @fn      pwm_rgb_Init
 *
 * @brief   Initialize PWM RGB module
 *********************************************************************/

void pwm_rgb_Init(byte task_id) {
    pwm_rgb_task_id = task_id;
    pwm5_initTimer1();
    
    // Initialize all channels to off
    pwm_rgb_set_color(0, 0, 0);
    pwm_rgb_set_channel(RETRO_VOLTMETER_CHANNEL, 128);
    
    // Set power management mode to conserve
    osal_pwrmgr_task_state(pwm_rgb_task_id, PWRMGR_CONSERVE);
}

/*********************************************************************
 * @fn      pwm_rgb_event_loop
 *
 * @brief   Event loop for PWM RGB module
 *********************************************************************/

uint16 pwm_rgb_event_loop(uint8 task_id, uint16 events) {
    (void)task_id;
    (void)events;
    
    // No events to process
    return 0;
}

/*********************************************************************
 * @fn      pwm_rgb_set_color
 *
 * @brief   Set RGB color using PWM
 *********************************************************************/

void pwm_rgb_set_color(uint8 red, uint8 green, uint8 blue) {
    PWM5(PWM_DEFAULT_FREQ, blue, PWM_CHANNEL_BLUE);
    PWM5(PWM_DEFAULT_FREQ, green, PWM_CHANNEL_GREEN);
    PWM5(PWM_DEFAULT_FREQ, red, PWM_CHANNEL_RED);
}

/*********************************************************************
 * @fn      pwm_rgb_set_channel
 *
 * @brief   Set single PWM channel duty cycle
 *********************************************************************/

void pwm_rgb_set_channel(uint8 channel, uint8 duty) {
    PWM5(PWM_DEFAULT_FREQ, duty, channel);
}

/*********************************************************************
 * @fn      PWM5
 *
 * @brief   Configure PWM output on specified channel
 *
 * @param   freq    - PWM frequency in Hz
 * @param   tduty   - Duty cycle value (0-255)
 * @param   channal - PWM channel number (1-4)
 *********************************************************************/

void PWM5(uint32 freq, uint16 tduty, uint8 channal) {
    // Suspend Timer 1 operation
    T1CTL &= ~(BV(1) | BV(0));
    
    // Calculate period value based on frequency
    uint16 tfreq;
    if (freq <= 32768) {
        tfreq = (uint16)(2000000 / freq);
    } else {
        tfreq = (uint16)(16000000 / freq);
    }
    
    // Set PWM signal period
    T1CC0L = (uint8)tfreq;
    T1CC0H = (uint8)(tfreq >> 8);
    
    // Configure duty cycle for selected channel
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
    
    // Start Timer 1 in up-down mode with appropriate clock divider
    if (freq <= 32768) {
        // Up-down mode, clock / 8
        T1CTL |= (BV(2) | 0x03);
    } else {
        // Up-down mode, clock / 1
        T1CTL = 0x03;
    }
}
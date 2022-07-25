/**
  ******************************************************************************
  * @file    led_driver.h
  * @author  CZ.NIC, z.s.p.o.
  * @date    22-July-2015
  * @brief   Header file led driver.
  ******************************************************************************
  ******************************************************************************
**/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LED_DRIVER_H
#define __LED_DRIVER_H

#define LED_COUNT		12

enum colours {
    WHITE_COLOUR        = 0xFFFFFF,
    RED_COLOUR          = 0xFF0000,
    GREEN_COLOUR        = 0x00FF00,
    BLUE_COLOUR         = 0x0000FF,
    BLACK_COLOUR        = 0x000000,
    YELLOW_COLOUR       = 0xFFFF00,
};

typedef enum led_modes {
    LED_DEFAULT_MODE   = 0,
    LED_USER_MODE      = 1,
}led_mode_t;

typedef enum led_states {
    LED_OFF         = 0,
    LED_ON          = 1,
}led_state_t;

enum led_numbers {
    LED0            = 0,
    LED1            = 1,
    LED2            = 2,
    LED3            = 3,
    LED4            = 4,
    LED5            = 5,
    LED6            = 6,
    LED7            = 7,
    LED8            = 8,
    LED9            = 9,
    LED10           = 10,
    LED11           = 11
};

enum led_names {
    POWER_LED       = LED11,
    LAN0_LED        = LED10,
    LAN1_LED        = LED9,
    LAN2_LED        = LED8,
    LAN3_LED        = LED7,
    LAN4_LED        = LED6,
    WAN_LED         = LED5,
    PCI1_LED        = LED3,
    PCI2_LED        = LED4,
    MSATA_PCI_LED   = LED2,
    USER_LED1       = LED1,
    USER_LED2       = LED0
};
/* PCI1 and PCI2 leds are reversed, there is a difference between numbering in schematic
editor and numbering on the case for the router */

typedef struct led_rgb_data {
    uint8_t blue; /* [0..255] */
    uint8_t green;
    uint8_t red;
}led_rgb_data_t;

struct led_rgb {
    led_rgb_data_t          led_rgb_default;    /* colour data */
    led_state_t             led_state_default;  /* LED ON/OFF default mode */
    led_state_t             led_state_user;     /* LED ON/OFF user mode */
    led_mode_t              led_mode;           /* default / user mode */
    uint16_t                brightness;
};

extern struct led_rgb leds[LED_COUNT];
extern uint8_t effect_reset_finished;

/*******************************************************************************
  * @function   led_driver_config
  * @brief      Configure LED driver.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void led_driver_config(void);

/*******************************************************************************
  * @function   led_driver_set_colour
  * @brief      Save colour of LED specified in parameters to be displayed in next cycle.
  * @param      led_index: position of LED (0..11) or index >=12 -> all LEDs
  * @param      colour: LED colour (RGB range).
  * @retval     None.
  *****************************************************************************/
void led_driver_set_colour(const uint8_t led_index, const uint32_t colour);

/*******************************************************************************
  * @function   led_driver_send_frame
  * @brief      Send frame to LED driver. It is called in timer interrupt.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void led_driver_send_frame(void);

/*******************************************************************************
  * @function   led_driver_pwm_set_brightness
  * @brief      Set PWM value.
  * @param      procent_val: PWM value in [%].
  * @retval     None.
  *****************************************************************************/
void led_driver_pwm_set_brightness(uint16_t procent_val);

/*******************************************************************************
  * @function   led_driver_pwm_get_brightness
  * @brief      Set PWM value.
  * @param      None.
  * @retval     procent_val: PWM value in [%].
  *****************************************************************************/
uint16_t led_driver_pwm_get_brightness(void);

/*******************************************************************************
  * @function   led_driver_step_brightness
  * @brief      Decrease LED brightness by 10% each step (each function call).
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void led_driver_step_brightness(void);

/*******************************************************************************
  * @function   led_driver_set_led_mode
  * @brief      Set mode to LED(s) - default or user mode
  * @param      led_index: position of LED (0..11) or led_index >=12 -> all LED.
  * @parame     led_mode: LED_DEFAULT_MODE / LED_USER_MODE
  * @retval     None.
  *****************************************************************************/
void led_driver_set_led_mode(const uint8_t led_index, const led_mode_t led_mode);

/*******************************************************************************
  * @function   led_driver_set_led_state
  * @brief      Set state of the LED(s) - LED_ON / LED_OFF
  * @param      led_index: position of LED (0..11) or led_index >=12 -> all LED.
  * @parame     led_state: LED_OFF / LED_ON
  * @retval     None.
  *****************************************************************************/
void led_driver_set_led_state(const uint8_t led_index, const led_state_t led_state);

/*******************************************************************************
  * @function   led_driver_set_led_state
  * @brief      Set state of the LED(s) from user/I2C - LED_ON / LED_OFF
  * @param      led_index: position of LED (0..11) or led_index >=12 -> all LED.
  * @parame     led_state: LED_OFF / LED_ON
  * @retval     None.
  *****************************************************************************/
void led_driver_set_led_state_user(const uint8_t led_index, const led_state_t led_state);

/*******************************************************************************
  * @function   led_driver_knight_rider_effect
  * @brief      Display knight rider effect on LEDs.
  * @param      colour: colour in RGB range.
  * @retval     None.
  *****************************************************************************/
void led_driver_knight_rider_effect(uint32_t colour);

/*******************************************************************************
  * @function   led_driver_knight_rider_colour_effect
  * @brief      Display knight rider effect on LEDs.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void led_driver_knight_rider_colour_effect(void);

/*******************************************************************************
  * @function   led_driver_double_knight_rider_effect
  * @brief      Display double knight rider effect on LEDs.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void led_driver_double_knight_rider_effect(void);

/*******************************************************************************
  * @function   led_driver_knight_rider_effect_handler
  * @brief      Display knight rider effect on LEDs during startup (called in
  *             timer interrupt).
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void led_driver_knight_rider_effect_handler(void);

/*******************************************************************************
  * @function   led_driver_bootloader_effect_handler
  * @brief      Display bootloader effect.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void led_driver_bootloader_effect_handler(void);

/*******************************************************************************
  * @function   led_driver_reset_effect
  * @brief      Enable/Disable knight rider effect after reset.
  * @param      colour: colour in RGB range.
  * @retval     None.
  *****************************************************************************/
void led_driver_reset_effect(FunctionalState state);

#endif /*__LED_DRIVER_H */

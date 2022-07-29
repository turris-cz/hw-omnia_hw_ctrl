/**
 ******************************************************************************
 * @file    led_driver.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    21-July-2015
 * @brief   LED RGB driver for Turris Omnia
 ******************************************************************************
 ******************************************************************************
 **/
/* Includes ------------------------------------------------------------------*/
#include "led_driver.h"
#include "delay.h"
#include "power_control.h"
#include "gpio.h"
#include "spi.h"
#include "timer.h"

/* Private define ------------------------------------------------------------*/
#define LED_SPI_ALT_FN		0
#define LED_SPI_MOSI_PIN	PIN(A, 7)
#define LED_SPI_SCK_PIN		PIN(A, 5)
#define LED_SPI_SS_PIN		PIN(A, 4)

#define LED_PWM_ALT_FN		0
#define LED_PWM_PIN		PIN(A, 3)

#define COLOUR_LEVELS               64
#define COLOUR_DECIMATION           2
#define MAX_LED_BRIGHTNESS          100
#define MAX_BRIGHTNESS_STEPS        8
#define EFFECT_TIMEOUT              5

/*******************************************************************************
// PWM Settings (Frequency and range)
//------------------------------------------------------------------------------
// period      = range (max = 0xFFFF => 16bit)
// Basic freq. = (APB2=48MHz) => TIM_CLK=48MHz
// period range    : 0 to 0xFFFF
// prescaler range : 0 to 0xFFFF
//
// PWM-Frq     = TIM_CLK/(period+1)/(prescaler+1)
*******************************************************************************/
#define LED_PWM_PERIOD		2000
#define LED_PWM_PRESCALE	6

/* Private typedef -----------------------------------------------------------*/
typedef enum rgb_colour {
    RED     = 0,
    GREEN   = 1,
    BLUE    = 2,
    WHITE   = -1,
}rgb_colour_t;

typedef enum led_effect_states {
    EFFECT_INIT,
    EFFECT_UP,
    EFFECT_DOWN,
    EFFECT_LEDSON,
    EFFECT_DEINIT
} effect_state_t;

struct led_rgb leds[LED_COUNT];
uint8_t effect_reset_finished; /* flag is set when LED effect after reset is
finished and normal operation can take the LED control */

/* values for LED brightness [%] */
static const uint16_t brightness_value[] = {100, 70, 40, 25, 12, 5, 1, 0};

/* Private functions ---------------------------------------------------------*/
static void led_driver_timer_config_knight_rider(void);

/*******************************************************************************
  * @function   led_driver_spi_config
  * @brief      SPI config for led driver serial register.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void led_driver_spi_config(void)
{
    spi_init(LED_SPI);
}

/*******************************************************************************
  * @function   led_driver_io_config
  * @brief      GPIO config for led driver serial register.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void led_driver_io_config(void)
{
    /* Configure SCK and MOSI pins */
    gpio_init_alts(LED_SPI_ALT_FN, pin_pushpull, pin_spd_3, pin_pulldown,
                   LED_SPI_SCK_PIN, LED_SPI_MOSI_PIN);

    /* Configure NSS pin (start with low value to hold previous data) */
    gpio_init_outputs(pin_pushpull, pin_spd_3, 0, LED_SPI_SS_PIN);
}

/*******************************************************************************
  * @function   led_driver_timer_config
  * @brief      Timer config for led driver serial register - send data regularly.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void led_driver_timer_config(void)
{
    timer_init(LED_TIMER, timer_interrupt, 200, 20, 4);
    timer_enable(LED_TIMER, 1);
}

/*******************************************************************************
  * @function   led_driver_set_colour
  * @brief      Set colour of LED specified in parameters to be displayed in next cycle.
  * @param      led_index: position of LED (0..11) or index >=12 -> all LEDs
  * @param      colour: LED colour (RGB range).
  * @retval     None.
  *****************************************************************************/
void led_driver_set_colour(const uint8_t led_index, const uint32_t colour)
{
    uint8_t idx;
    struct led_rgb *rgb_leds = leds;

    if (led_index >= LED_COUNT) /* all LEDs */
    {
        for (idx = 0; idx < LED_COUNT; idx++, rgb_leds++)
        {
            rgb_leds->led_rgb_default.red = colour >> 16;
            rgb_leds->led_rgb_default.green = (colour >> 8) & 0xFF;
            rgb_leds->led_rgb_default.blue = colour & 0xFF;
        }
    }
    else /* individual LED */
    {
        rgb_leds += led_index;

        rgb_leds->led_rgb_default.red = colour >> 16;
        rgb_leds->led_rgb_default.green = (colour >> 8) & 0xFF;
        rgb_leds->led_rgb_default.blue = colour & 0xFF;
    }
}

/*******************************************************************************
  * @function   led_driver_send_data16b
  * @brief      Send SPI data to the LED driver.
  * @param      data: 16bit data buffer.
  * @retval     None.
  *****************************************************************************/
static void led_driver_send_data16b(const uint16_t data)
{
    spi_send16(LED_SPI, data);
    while (spi_is_busy(LED_SPI));
}

/*******************************************************************************
  * @function   led_driver_prepare_data
  * @brief      Prepare data to be sent to LED driver.
  * @param      colour: RED, GREEN or BLUE.
  * @param      current_colour_level: colour density [0..255]
  * @retval     Data to be sent to LED driver.
  *****************************************************************************/
static uint16_t led_driver_prepare_data(const rgb_colour_t colour, const uint8_t current_colour_level)
{
    uint16_t data = 0;
    uint8_t idx;
    struct led_rgb *rgb_leds = leds;

    switch (colour)
    {
        case RED:
        {
            for (idx = 0; idx < LED_COUNT; idx++, rgb_leds++)
            {
                if (rgb_leds->led_mode == LED_DEFAULT_MODE)
                {
                    if (rgb_leds->led_state_default == LED_ON)
                    {
                        if (rgb_leds->led_rgb_default.red > current_colour_level)
                        {
                            data |= 1 << (2 + idx); /* shift by 2 - due to the HW connection */
                        }
                    }
                }
                else /* LED_USER_MODE has the same colour profile as default mode now */
                {
                    if (rgb_leds->led_state_user == LED_ON)
                    {
                        if (rgb_leds->led_rgb_default.red > current_colour_level)
                        {
                            data |= 1 << (2 + idx);
                        }
                    }
                }
            }
        } break;

        case GREEN:
        {
            for (idx = 0; idx < LED_COUNT; idx++, rgb_leds++)
            {
                if (rgb_leds->led_mode == LED_DEFAULT_MODE)
                {
                    if (rgb_leds->led_state_default == LED_ON)
                    {
                        if (rgb_leds->led_rgb_default.green > current_colour_level)
                        {
                            data |= 1 << (2 + idx);
                        }
                    }
                }
                else /* LED_USER_MODE has the same colour profile as default mode now */
                {
                    if (rgb_leds->led_state_user == LED_ON)
                    {
                        if (rgb_leds->led_rgb_default.green > current_colour_level)
                        {
                            data |= 1 << (2 + idx);
                        }
                    }
                }
            }
        } break;

        case BLUE:
        {
            for (idx = 0; idx < LED_COUNT; idx++, rgb_leds++)
            {
                if (rgb_leds->led_mode == LED_DEFAULT_MODE)
                {
                    if (rgb_leds->led_state_default == LED_ON)
                    {
                        if (rgb_leds->led_rgb_default.blue > current_colour_level)
                        {
                            data |= 1 << (2 + idx);
                        }
                    }
                }
                else /* LED_USER_MODE has the same colour profile as default mode now */
                {
                    if (rgb_leds->led_state_user == LED_ON)
                    {
                        if (rgb_leds->led_rgb_default.blue > current_colour_level)
                        {
                            data |= 1 << (2 + idx);
                        }
                    }
                }
            }
        } break;

        default:
            break;
    }

    return data;
}

/*******************************************************************************
  * @function   led_driver_send_frame
  * @brief      Send frame to LED driver. It is called in LED_TIMER interrupt.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void led_driver_send_frame(void)
{
    static uint8_t level;
    uint16_t data;
    static rgb_colour_t colour = RED;

    switch (colour)
    {
        case RED:
        {
            /* decrease 255 colour levels to COLOUR_LEVELS by shift (COLOUR_DECIMATION) */
            data = led_driver_prepare_data(RED, level << COLOUR_DECIMATION);
            colour = GREEN;
        } break;

        case GREEN:
        {
            data = led_driver_prepare_data(GREEN, level << COLOUR_DECIMATION);
            colour = BLUE;
        } break;

        case BLUE: /* last colour -> go back to RED */
        {
            data = led_driver_prepare_data(BLUE, level << COLOUR_DECIMATION);
            colour = RED;
        } break;

        default:
            unreachable();
    }

    led_driver_send_data16b(data);

    /* blue colour were sent to driver -> enable latch to write to LEDs */
    if (colour == RED)
    {
        /* latch enable pulse */
        gpio_write(LED_SPI_SS_PIN, 1);
        __NOP();
        gpio_write(LED_SPI_SS_PIN, 0);

        level++;

        /* restart cycle - all levels were sent to driver */
        if (level >= COLOUR_LEVELS)
            level = 0;
    }
}

void __irq led_driver_irq_handler(void)
{
	led_driver_send_frame();
	timer_irq_clear(LED_TIMER);
}

/*******************************************************************************
  * @function   led_driver_pwm_io_config
  * @brief      Config of PWM signal for LED driver.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void led_driver_pwm_io_config(void)
{
    gpio_init_alts(LED_PWM_ALT_FN, pin_pushpull, pin_spd_1, pin_pullup,
                   LED_PWM_PIN);
}

/*******************************************************************************
  * @function   led_driver_pwm_timer_config
  * @brief      Timer config for PWM signal (OE pin of LED driver).
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void led_driver_pwm_timer_config(void)
{
    timer_init(LED_PWM_TIMER, timer_pwm, LED_PWM_PERIOD, LED_PWM_PRESCALE, 0);
    timer_enable(LED_PWM_TIMER, 1);
}

/*******************************************************************************
  * @function   led_driver_pwm_config
  * @brief      Configuration of PWM functionality.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void led_driver_pwm_config(void)
{
    led_driver_pwm_io_config();
    led_driver_pwm_timer_config();
}

/*******************************************************************************
  * @function   led_driver_init_led
  * @brief      Enable all LED to default mode (LEDs are ON).
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void led_driver_init_led(void)
{
    uint8_t idx;
    struct led_rgb *rgb_leds = leds;

    /* user mode - all LEDs white and ON */
    for (idx = 0; idx < LED_COUNT; idx++, rgb_leds++)
    {
        rgb_leds->led_state_user = LED_ON;
        rgb_leds->led_mode = LED_USER_MODE;
    }
    led_driver_set_colour(LED_COUNT, WHITE_COLOUR); /* all LEDs white */

    /* default mode - all LEDS OFF and white */
    for (idx = 0, rgb_leds = leds; idx < LED_COUNT; idx++, rgb_leds++)
    {
        rgb_leds->led_state_default = LED_OFF;
        rgb_leds->led_mode = LED_DEFAULT_MODE;
    }
    led_driver_set_colour(LED_COUNT, WHITE_COLOUR); /* all LEDs white */
}

/*******************************************************************************
  * @function   led_driver_config
  * @brief      Configure LED driver.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void led_driver_config(void)
{
    led_driver_io_config();
    led_driver_spi_config();

    led_driver_init_led(); /* set mode and state - default after reset */

    led_driver_pwm_config();
    led_driver_pwm_set_brightness(MAX_LED_BRIGHTNESS); /* 100% brightness after reset */

    led_driver_timer_config();
    led_driver_timer_config_knight_rider();
}

/*******************************************************************************
  * @function   led_driver_pwm_set_brightness
  * @brief      Set PWM value.
  * @param      procent_val: PWM value in [%].
  * @retval     None.
  *****************************************************************************/
void led_driver_pwm_set_brightness(uint16_t procent_val)
{
    uint16_t counter_val;
    struct led_rgb *rgb_leds = leds;

    if (procent_val > MAX_LED_BRIGHTNESS)
        procent_val = MAX_LED_BRIGHTNESS;

    counter_val = procent_val * LED_PWM_PERIOD / MAX_LED_BRIGHTNESS;

    timer_set_pulse(LED_PWM_TIMER, counter_val);
    rgb_leds->brightness = procent_val;
}

/*******************************************************************************
  * @function   led_driver_pwm_get_brightness
  * @brief      Set PWM value.
  * @param      None.
  * @retval     procent_val: PWM value in [%].
  *****************************************************************************/
uint16_t led_driver_pwm_get_brightness(void)
{
    struct led_rgb *rgb_leds = leds;

    return  rgb_leds->brightness;
}

/*******************************************************************************
  * @function   led_driver_step_brightness
  * @brief      Decrease LED brightness by step (each function call).
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void led_driver_step_brightness(void)
{
    struct led_rgb *rgb_leds = leds;
    static uint8_t step = 1;

    rgb_leds->brightness = brightness_value[step++];
    led_driver_pwm_set_brightness(rgb_leds->brightness);

    if (step >= MAX_BRIGHTNESS_STEPS)
        step = 0;
}

/*******************************************************************************
  * @function   led_driver_set_led_mode
  * @brief      Set mode to LED(s) - default or user mode
  * @param      led_index: position of LED (0..11) or led_index >=12 -> all LED.
  * @parame     led_mode: LED_DEFAULT_MODE / LED_USER_MODE
  * @retval     None.
  *****************************************************************************/
void led_driver_set_led_mode(const uint8_t led_index, const led_mode_t led_mode)
{
    uint8_t idx;
    struct led_rgb *rgb_leds = leds;

    if (led_index >= LED_COUNT)  /* all LED */
    {
        for (idx = 0; idx < LED_COUNT; idx++, rgb_leds++)
        {
            rgb_leds->led_mode = led_mode;
        }
    }
    else /* or individual LED */
    {
        rgb_leds += led_index;
        rgb_leds->led_mode = led_mode;
    }
}

/*******************************************************************************
  * @function   led_driver_set_led_state
  * @brief      Set state of the LED(s) - LED_ON / LED_OFF
  * @param      led_index: position of LED (0..11) or led_index >=12 -> all LED.
  * @parame     led_state: LED_OFF / LED_ON
  * @retval     None.
  *****************************************************************************/
void led_driver_set_led_state(const uint8_t led_index, const led_state_t led_state)
{
    uint8_t idx;
    struct led_rgb *rgb_leds = leds;

    if (led_index >= LED_COUNT) /* all LED */
    {
        for (idx = 0; idx < LED_COUNT; idx++, rgb_leds++)
        {
            if (rgb_leds->led_mode == LED_DEFAULT_MODE)
            {
                rgb_leds->led_state_default = led_state;
            }
            else
            {
                rgb_leds->led_state_user = led_state;
            }
        }
    }
    else /* or individual LED */
    {
        rgb_leds += led_index;

        if (rgb_leds->led_mode == LED_DEFAULT_MODE)
            {
                rgb_leds->led_state_default = led_state;
            }
            else
            {
                rgb_leds->led_state_user = led_state;
            }
    }
}

/*******************************************************************************
  * @function   led_driver_set_led_state
  * @brief      Set state of the LED(s)i from user/I2C - LED_ON / LED_OFF
  * @param      led_index: position of LED (0..11) or led_index >=12 -> all LED.
  * @parame     led_state: LED_OFF / LED_ON
  * @retval     None.
  *****************************************************************************/
void led_driver_set_led_state_user(const uint8_t led_index, const led_state_t led_state)
{
    struct led_rgb *rgb_leds;
    int8_t idx;

    if (led_index >= LED_COUNT) { /* all LED */
        for (idx = 0; idx < LED_COUNT; idx++, rgb_leds++) {
            rgb_leds = leds + idx;
            if (rgb_leds->led_mode == LED_DEFAULT_MODE)
                rgb_leds->led_state_user = led_state;
            else
                led_driver_set_led_state(led_index, led_state);
        }
    } else {
        rgb_leds = leds + led_index;
        if (rgb_leds->led_mode == LED_DEFAULT_MODE)
            rgb_leds->led_state_user = led_state;
        else
            led_driver_set_led_state(led_index, led_state);
    }
}

/*******************************************************************************
  * @function   led_driver_knight_rider_effect
  * @brief      Display knight rider effect on LEDs.
  * @param      colour: colour in RGB range.
  * @retval     None.
  *****************************************************************************/
void led_driver_knight_rider_effect(uint32_t colour)
{
    int8_t led;

    led_driver_set_led_state(LED_COUNT, LED_OFF);
    led_driver_set_colour(LED_COUNT, colour);
    led_driver_set_led_state(LED0, LED_ON);

    for (led = LED1; led < LED_COUNT; led++)
    {
        delay(70);
        led_driver_set_led_state(led - 1, LED_OFF);
        led_driver_set_led_state(led, LED_ON);
    }

    for (led = LED10; led > -1 ; led--)
    {
        delay(70);
        led_driver_set_led_state(led + 1, LED_OFF);
        led_driver_set_led_state(led, LED_ON);
    }

    led_driver_set_colour(LED_COUNT, WHITE_COLOUR);
    led_driver_set_led_state(LED_COUNT, LED_OFF);
}

/*******************************************************************************
  * @function   led_driver_knight_rider_colour_effect
  * @brief      Display knight rider effect on LEDs.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void led_driver_knight_rider_colour_effect(void)
{
    int8_t colour;

    led_driver_set_led_state(LED_COUNT, LED_OFF);

    for (colour = WHITE; colour < BLUE + 1; colour++)
    {
        switch (colour)
        {
        case RED: led_driver_knight_rider_effect(RED_COLOUR); break;
        case GREEN: led_driver_knight_rider_effect(GREEN_COLOUR); break;
        case BLUE: led_driver_knight_rider_effect(BLUE_COLOUR); break;
        default: led_driver_knight_rider_effect(WHITE_COLOUR); break;
        }
    }
}


/*******************************************************************************
  * @function   led_driver_double_knight_rider_effect
  * @brief      Display double knight rider effect on LEDs.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void led_driver_double_knight_rider_effect(void)
{
    int8_t led_down, led_up;
    int8_t colour;

    led_driver_set_led_state(LED_COUNT, LED_OFF);
    led_driver_set_colour(LED_COUNT, WHITE_COLOUR);

    for (colour = WHITE; colour < BLUE + 1; colour++)
    {
        switch (colour)
        {
        case RED: led_driver_set_colour(LED_COUNT, RED_COLOUR); break;
        case GREEN: led_driver_set_colour(LED_COUNT, GREEN_COLOUR); break;
        case BLUE: led_driver_set_colour(LED_COUNT, BLUE_COLOUR); break;
        default: led_driver_set_colour(LED_COUNT, WHITE_COLOUR); break;
        }

        led_driver_set_led_state(LED5, LED_ON);
        led_driver_set_led_state(LED6, LED_ON);

        for(led_down = LED4, led_up = LED7; led_down > -1; led_down--, led_up++)
        {
            delay(100);
            led_driver_set_led_state(led_down + 1, LED_OFF);
            led_driver_set_led_state(led_down, LED_ON);

            led_driver_set_led_state(led_up - 1, LED_OFF);
            led_driver_set_led_state(led_up, LED_ON);
        }

        for(led_down = LED10, led_up = LED1; led_down > LED5; led_down--, led_up++)
        {
            delay(100);
            led_driver_set_led_state(led_down + 1, LED_OFF);
            led_driver_set_led_state(led_down, LED_ON);

            led_driver_set_led_state(led_up - 1, LED_OFF);
            led_driver_set_led_state(led_up, LED_ON);
        }
    }

    led_driver_set_colour(LED_COUNT, 0xFFFFFF); //back to default colour
    led_driver_set_led_state(LED_COUNT, LED_OFF);
}

/*******************************************************************************
  * @function   led_driver_timer_config_knight_rider
  * @brief      Timer config for knight rider effect after reset.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void led_driver_timer_config_knight_rider(void)
{
    timer_init(LED_EFFECT_TIMER, timer_interrupt, 8000, 400, 5);
}

/*******************************************************************************
  * @function   led_driver_reset_effect
  * @brief      Enable/Disable knight rider effect after reset.
  * @param      colour: colour in RGB range.
  * @retval     None.
  *****************************************************************************/
void led_driver_reset_effect(bool state)
{
    timer_enable(LED_EFFECT_TIMER, state);
    if (!state)
        timer_set_counter(LED_EFFECT_TIMER, 0);
}

/*******************************************************************************
  * @function   led_driver_knight_rider_effect_handler
  * @brief      Display knight rider effect on LEDs during startup (called in
  *             timer interrupt).
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void led_driver_knight_rider_effect_handler(void)
{
    static int8_t led;
    static uint8_t state_timeout_cnt;
    static effect_state_t effect_state; /* states for LED effect after reset */

    switch (effect_state)
    {
        case EFFECT_INIT:
        {
            effect_reset_finished = RESET;
            led_driver_set_led_state(LED_COUNT, LED_OFF);
            led_driver_set_colour(LED_COUNT, WHITE_COLOUR);            
            led_driver_set_led_state(LED0, LED_ON);
            effect_state = EFFECT_UP;
        } break;

        case EFFECT_UP:
        {
            led++;
            led_driver_set_led_state(LED11, LED_OFF);
            led_driver_set_led_state(led - 1, LED_OFF);
            led_driver_set_led_state(led, LED_ON);

            if (led >= LED11)
            {
                effect_state = EFFECT_DOWN; /* next state */
            }
            else
            {
                effect_state = EFFECT_UP;
            }
        } break;

        case EFFECT_DOWN:
        {
            led--;
            led_driver_set_led_state(led + 1, LED_OFF);
            led_driver_set_led_state(led, LED_ON);

            if (led <= 0)
            {
                effect_state = EFFECT_LEDSON; /* next state */
            }
            else
            {
                effect_state = EFFECT_DOWN;
            }
        } break;

        case EFFECT_LEDSON:
        {
            led_driver_set_led_state(LED_COUNT, LED_ON);
            led_driver_set_colour(LED_COUNT, GREEN_COLOUR | BLUE_COLOUR);
            effect_state = EFFECT_DEINIT;
        } break;

        case EFFECT_DEINIT:
        {
            state_timeout_cnt++;

            if (state_timeout_cnt >= EFFECT_TIMEOUT)
            {
                led_driver_set_led_state(LED_COUNT, LED_OFF);
                led_driver_set_colour(LED_COUNT, WHITE_COLOUR);

                led_driver_set_led_mode(LED_COUNT, LED_DEFAULT_MODE);
                led_driver_reset_effect(DISABLE);
                state_timeout_cnt = 0;
                effect_reset_finished = SET;
                effect_state = EFFECT_INIT;
            }
            else
            {
                effect_state = EFFECT_DEINIT;
            }

        } break;
    }
}

/*******************************************************************************
  * @function   led_driver_bootloader_effect_handler
  * @brief      Display bootloader effect.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void led_driver_bootloader_effect_handler(void)
{
    static uint8_t timeout;
    static bool on;

    timeout++;

    if (timeout >= 8) {
        timeout = 0;

        on = !on;
        led_driver_set_led_state(LED_COUNT, on);
    }
}

void __irq led_driver_effect_irq_handler(void)
{
	if (BOOTLOADER_BUILD)
		led_driver_bootloader_effect_handler();
	else
		led_driver_knight_rider_effect_handler();

	timer_irq_clear(LED_EFFECT_TIMER);
}

/**
 ******************************************************************************
 * @file    debounce.h
 * @author  CZ.NIC, z.s.p.o.
 * @date    21-July-2015
 * @brief   Header file for debounce.c
 ******************************************************************************
 ******************************************************************************
 **/
#ifndef __DEBOUNCE_H
#define __DEBOUNCE_H

#define MAX_BUTTON_PRESSED_COUNTER      7
#define MAX_BUTTON_DEBOUNCE_STATE       3

typedef enum button_modes {
    BUTTON_DEFAULT,
    BUTTON_USER,
}button_mode_t;

typedef enum button_states {
    BUTTON_PRESSED,
    BUTTON_RELEASED,
}button_state_t;

enum input_states {
    DEACTIVATED = 0,
    ACTIVATED = 1, /* when signal reaches a defined treshold */
};

/* flags of input signals */
struct input_sig {
    uint8_t man_res     :1;
    uint8_t sysres_out  :1;
    uint8_t dbg_res     :1;
    uint8_t m_res       :1;
    uint8_t pg          :1;
    uint8_t pg_4v5      :1;
    uint8_t usb30_ovc   :1;
    uint8_t usb31_ovc   :1;
    uint8_t rtc_alarm   :1;
    uint8_t button_sts  :1;
    uint8_t card_det    :1;
    uint8_t msata_ind   :1;
};

struct button_def {
    button_mode_t button_mode;
    button_state_t button_state;
    int8_t button_pressed_counter;
    uint16_t button_pin_state[MAX_BUTTON_DEBOUNCE_STATE];
    uint16_t button_debounce_state;
};

extern struct input_sig debounce_input_signal;
extern struct button_def button_front;

/*******************************************************************************
  * @function   debounce_config
  * @brief      Debouncer configuration.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void debounce_config(void);

/*******************************************************************************
  * @function   debounce_timer_irq_handler
  * @brief      Main debounce function. Called as timer interrupt handler.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void debounce_timer_irq_handler(void);

/*******************************************************************************
  * @function   debounce_check_inputs
  * @brief      Check input signal.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void debounce_check_inputs(void);

/*******************************************************************************
  * @function   button_counter_increase
  * @brief      Increase button counter.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void button_counter_increase(void);

/*******************************************************************************
  * @function   button_counter_decrease
  * @brief      Decrease button counter by the current value in i2c status structure.
  * @param      value: decrease the button counter by this parameter
  * @retval     None.
  *****************************************************************************/
void button_counter_decrease(uint8_t value);

#endif // __DEBOUNCE_H

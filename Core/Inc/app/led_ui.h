/*
 * led_ui.h
 *
 *  Created on: Jan 26, 2026
 *      Author: feder
 */

#ifndef LED_UI_H
#define LED_UI_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* Eventos visuales */
typedef enum {
  LED_EVENT_NONE = 0,
  LED_EVENT_LETTER,   // flash corto
  LED_EVENT_SPACE,    // flash largo
  LED_EVENT_ERROR     // doble flash
} led_event_t;

/* Inicializa el LED externo (GPIO ya configurado como Output) */
void LedUI_Init(GPIO_TypeDef *port, uint16_t pin);

/* Se llama en el superloop (no bloqueante) */
void LedUI_Task(uint32_t now);

/* Estado continuo (por ejemplo: luz detectada => LED fijo encendido) */
void LedUI_SetLevel(uint8_t level);

/* Dispara un evento visual (flash) */
void LedUI_Event(led_event_t ev);

#endif

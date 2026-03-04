/*
 * led_ui.c
 *
 *  Created on: Jan 26, 2026
 *      Author: feder
 */

#include "app/led_ui.h"

/* Tiempos (ms) */
#define LED_SHORT_MS   80U
#define LED_LONG_MS    250U
#define LED_GAP_MS     80U

static GPIO_TypeDef *g_port = NULL;
static uint16_t g_pin = 0;

/* Nivel fijo (estado continuo) */
static uint8_t g_level = 0;

/* Patrón de evento */
static uint8_t pattern_active = 0;
static led_event_t current_ev = LED_EVENT_NONE;

static uint8_t step = 0;
static uint32_t t_next = 0;

static void led_write(uint8_t on)
{
  if (!g_port) return;
  HAL_GPIO_WritePin(g_port, g_pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void LedUI_Init(GPIO_TypeDef *port, uint16_t pin)
{
  g_port = port;
  g_pin = pin;

  g_level = 0;
  pattern_active = 0;
  current_ev = LED_EVENT_NONE;

  step = 0;
  t_next = 0;

  led_write(0);
}

void LedUI_SetLevel(uint8_t level)
{
  g_level = level ? 1U : 0U;

  /* Solo actualizamos el LED si NO hay un patrón (evento) en curso */
  if (!pattern_active)
    led_write(g_level);
}

void LedUI_Event(led_event_t ev)
{
  /* Un evento reinicia el patrón (prioridad alta) */
  pattern_active = 1;
  current_ev = ev;

  step = 0;
  t_next = HAL_GetTick();   /* arranca ya */
}

void LedUI_Task(uint32_t now)
{
  if (!pattern_active) return;
  if (now < t_next) return;

  /* Patrones:
     - LETRA:  ON corto
     - ESPACIO: ON largo
     - ERROR:  ON corto, OFF gap, ON corto
  */

  if (current_ev == LED_EVENT_LETTER)
  {
    if (step == 0) { led_write(1); t_next = now + LED_SHORT_MS; step = 1; return; }
    if (step == 1) { led_write(0); pattern_active = 0; LedUI_SetLevel(g_level); return; }
  }
  else if (current_ev == LED_EVENT_SPACE)
  {
    if (step == 0) { led_write(1); t_next = now + LED_LONG_MS; step = 1; return; }
    if (step == 1) { led_write(0); pattern_active = 0; LedUI_SetLevel(g_level); return; }
  }
  else if (current_ev == LED_EVENT_ERROR)
  {
    if (step == 0) { led_write(1); t_next = now + LED_SHORT_MS; step = 1; return; }
    if (step == 1) { led_write(0); t_next = now + LED_GAP_MS;  step = 2; return; }
    if (step == 2) { led_write(1); t_next = now + LED_SHORT_MS; step = 3; return; }
    if (step == 3) { led_write(0); pattern_active = 0; LedUI_SetLevel(g_level); return; }
  }

  /* fallback */
  led_write(0);
  pattern_active = 0;
  LedUI_SetLevel(g_level);
}

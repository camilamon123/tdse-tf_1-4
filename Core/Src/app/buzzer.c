/*
 * buzzer.c
 *
 *  Created on: Jan 25, 2026
 *      Author: feder
 */

#include "app/buzzer.h"

static GPIO_TypeDef *g_port = NULL;
static uint16_t g_pin = 0;

static uint8_t g_on = 0;
static uint32_t t_off = 0;

/*
 * Guarda la referencia al pin del buzzer y lo deja apagado.
 */
void Buzzer_Init(GPIO_TypeDef *port, uint16_t pin)
{
  g_port = port;
  g_pin = pin;

  if (g_port)
    HAL_GPIO_WritePin(g_port, g_pin, GPIO_PIN_RESET);

  g_on = 0;
  t_off = 0;
}

/*
 * Solicita un beep por 'ms' milisegundos.
 */

void Buzzer_Beep(uint32_t ms)
{
  if (!g_port) return;

  HAL_GPIO_WritePin(g_port, g_pin, GPIO_PIN_SET);
  g_on = 1;
  t_off = HAL_GetTick() + ms;
}

/*
 * si el buzzer está encendido y ya pasó el tiempo, lo apaga.
 */
void Buzzer_Task(uint32_t now)
{
  if (!g_port) return;

  if (g_on && ((int32_t)(now - t_off) >= 0))
  {
    HAL_GPIO_WritePin(g_port, g_pin, GPIO_PIN_RESET);
    g_on = 0;
  }
}

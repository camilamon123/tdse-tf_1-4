/*
 * button_input.c
 *
 *  Created on: Jan 21, 2026
 *      Author: feder
 */

#include "app/button_input.h"
#include "main.h"

#define BTN_PORT GPIOC
#define BTN_PIN  GPIO_PIN_13

#define DEBOUNCE_MS 20

static uint8_t stable = 0;
static uint8_t last_raw = 0;
static uint32_t t_change = 0;

/*
 * Inicialización del módulo:
 *  - Toma una primera lectura para fijar stable/last_raw.
 *  - Guarda el tick actual para la lógica de debounce.
 */
void ButtonInput_Init(void)
{
    last_raw = (HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN) == GPIO_PIN_RESET) ? 1 : 0;
    stable = last_raw;
    t_change = HAL_GetTick();
}

/*
 * lee el pin y actualiza el estado estable con debounce.
 * devuelve: stable (1=presionado, 0=suelto).
 */
uint8_t ButtonInput_GetStable(void)
{
    uint8_t raw = (HAL_GPIO_ReadPin(BTN_PORT, BTN_PIN) == GPIO_PIN_RESET) ? 1 : 0;
    uint32_t now = HAL_GetTick();

    if (raw != last_raw)
    {
        last_raw = raw;
        t_change = now;
    }

    if ((now - t_change) >= DEBOUNCE_MS)
    {
        stable = raw;
    }

    return stable;
}

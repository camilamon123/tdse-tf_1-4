/*
 * light_sensor.c
 *
 *  Created on: Jan 25, 2026
 *      Author: feder
 */

#include "app/light_sensor.h"

static uint16_t g_th_low  = 1200;
static uint16_t g_th_high = 1800;

static uint16_t g_th_low_default  = 1200;
static uint16_t g_th_high_default = 1800;

static uint8_t  g_level = 0;  // 0=OFF  1=ON

void LightSensor_Init(uint16_t th_low_default, uint16_t th_high_default)
{
  g_th_low_default  = th_low_default;
  g_th_high_default = th_high_default;

  g_th_low  = th_low_default;
  g_th_high = th_high_default;

  g_level = 0;
}

uint16_t LightSensor_GetThLow(void)  { return g_th_low;  }
uint16_t LightSensor_GetThHigh(void) { return g_th_high; }

int LightSensor_SetThresholds(uint16_t th_low, uint16_t th_high)
{
  if (th_high <= th_low + 10) return -1;  // evita rango inválido

  g_th_low  = th_low;
  g_th_high = th_high;
  return 0;
}

uint8_t LightSensor_GetLevel(void)
{
  return g_level;
}

uint8_t LightSensor_UpdateLevel(uint16_t value)
{
  /* Histéresis estable:
     - si estaba OFF entonces solo pasa a ON al superar th_high
     - si estaba ON  entonces solo pasa a OFF al bajar de th_low
  */
  if (g_level == 0)
  {
    if (value >= g_th_high) g_level = 1;
  }
  else
  {
    if (value <= g_th_low) g_level = 0;
  }

  return g_level;
}

uint8_t LightSensor_ComputePercent(uint16_t value)
{
  /* Si el rango no es válido, normaliza en 0..4095 */
  if (g_th_high <= g_th_low + 10)
  {
    if (value >= 4095) return 100;
    return (uint8_t)((value * 100U) / 4095U);
  }

  /* Normalización por thresholds */
  if (value <= g_th_low)  return 0;
  if (value >= g_th_high) return 100;

  uint32_t num = (uint32_t)(value - g_th_low) * 100U;
  uint32_t den = (uint32_t)(g_th_high - g_th_low);

  return (uint8_t)(num / den);
}

void LightSensor_SyncLevel(uint16_t value)
{
  if (value <= g_th_low) g_level = 0;
  else if (value >= g_th_high) g_level = 1;
  else g_level = 0; // zona muerta → arrancar en OFF evita glitch
}
/* ===================== CALIB ===================== */

void LightSensor_CalibStart(void)
{
  /* vuelve a defaults para arrancar limpio */
  g_th_low  = g_th_low_default;
  g_th_high = g_th_high_default;
  g_level   = 0;
}

int LightSensor_CalibSetLow(uint16_t value)
{
  /* Permitimos setear low aunque todavía no sepamos high, pero cuidamos el rango */
  if (g_th_high <= value + 10) return -1;
  g_th_low = value;
  return 0;
}

int LightSensor_CalibSetHigh(uint16_t value)
{
  if (value <= g_th_low + 10) return -1;
  g_th_high = value;
  return 0;
}

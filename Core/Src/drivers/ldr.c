/*
 * ldr.c
 *
 *  Created on: Jan 21, 2026
 *      Author: feder
 */

#include "drivers/ldr.h"
#include "main.h"

/* hadc1 está en main.c */
extern ADC_HandleTypeDef hadc1;

static uint8_t  g_inited    = 0;
static uint32_t g_filt_q8   = 0;     // filtro en Q8
static uint16_t g_last_raw  = 0;     // último raw leído
static uint8_t  g_filt_init = 0;     // bandera de primer sample

void LDR_Init(void)
{
  g_inited    = 1;
  g_filt_q8   = 0;
  g_last_raw  = 0;
  g_filt_init = 0;
}

uint16_t LDR_ReadRaw(void)
{
  if (!g_inited) LDR_Init();

  HAL_ADC_Start(&hadc1);

  if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK)
  {
    HAL_ADC_Stop(&hadc1);
    return g_last_raw;  // si falla, devolvemos el último
  }

  g_last_raw = (uint16_t)HAL_ADC_GetValue(&hadc1);

  HAL_ADC_Stop(&hadc1);
  return g_last_raw;
}

uint16_t LDR_ReadFiltered(void)
{
  uint16_t raw = LDR_ReadRaw();

  uint32_t raw_q8 = ((uint32_t)raw) << 8;

  /* Primer sample: inicializamos el filtro en raw */
  if (!g_filt_init) {
    g_filt_q8 = raw_q8;
    g_filt_init = 1;
    return raw;
  }

  /* EMA alpha=1/8 usando signed para evitar underflow */
  int32_t err = (int32_t)raw_q8 - (int32_t)g_filt_q8;
  int32_t next = (int32_t)g_filt_q8 + (err >> 3);

  /* Saturación opcional a rango ADC [0..4095] en Q8 */
  if (next < 0) next = 0;
  if (next > (int32_t)(4095u << 8)) next = (int32_t)(4095u << 8);

  g_filt_q8 = (uint32_t)next;

  return (uint16_t)(g_filt_q8 >> 8);
}

uint16_t LDR_GetLastRaw(void)
{
  return g_last_raw;
}

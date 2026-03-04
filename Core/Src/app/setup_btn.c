/*
 * setup_btn.c
 *
 *  Created on: Jan 26, 2026
 *      Author: feder
 */

#include "app/setup_btn.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "app/button_input.h"
#include "drivers/ldr.h"
#include "app/light_sensor.h"
#include "app/led_ui.h"
#include "app/bt_link.h"
#include "app/buzzer.h"
#include "app/nv_store.h"
#include "app/morse_decoder.h"

#define SETUP_BEEP_MS  60U

/* ===================== UART PRINT (local) ===================== */
static UART_HandleTypeDef *g_huart_setup = NULL;

static void setup_printf(const char *fmt, ...)
{
  char msg[128];

  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);

  if (n <= 0) return;

  size_t len = (size_t)n;
  if (len >= sizeof(msg)) len = sizeof(msg) - 1u;

  /* PC (USART2) */
  if (g_huart_setup) {
    HAL_UART_Transmit(g_huart_setup, (uint8_t*)msg, (uint16_t)len, 200);
  }

  /* BT (USART1 vía BtLink) */
  BtLink_Write((const uint8_t*)msg, len);
}

/* ===================== FSM SETUP ===================== */
typedef enum {
  SETUP_IDLE = 0,
  SETUP_WAIT_LOW,
  SETUP_WAIT_HIGH,
  SETUP_DONE
} setup_state_t;

static setup_state_t st = SETUP_IDLE;
static uint8_t prev_btn = 0;

/* helper: detecta "click" (flanco de subida) del botón estable */
static uint8_t btn_clicked(void)
{
  uint8_t b = ButtonInput_GetStable();   /* 1 = apretado, 0 = suelto */
  uint8_t click = (b == 1 && prev_btn == 0) ? 1 : 0;
  prev_btn = b;
  return click;
}

void SetupBtn_Init(UART_HandleTypeDef *huart)
{
  g_huart_setup = huart;
  st = SETUP_IDLE;
  prev_btn = ButtonInput_GetStable();
}

void SetupBtn_Start(uint32_t now)
{
  (void)now;

  st = SETUP_WAIT_LOW;
  prev_btn = ButtonInput_GetStable();

  setup_printf("\r\n[SETUP] Calibración con botón\r\n");
  setup_printf("Paso 1: Iluminá el LDR un poco por encima de la luz ambiente y apretá el botón\r\n");
  setup_printf("Paso 2: Iluminá el LDR más de cerca y apretá el botón\r\n\r\n");

  /* apagamos el LED indicador fijo al arrancar */
  LedUI_SetLevel(0);
}

void SetupBtn_Task(uint32_t now)
{
  if (st == SETUP_IDLE || st == SETUP_DONE)
    return;

  /* ===================== Feedback visual durante SETUP =====================
   * - WAIT_LOW  : parpadeo lento
   * - WAIT_HIGH : parpadeo rápido
   */
  {
    uint16_t filt = LDR_ReadFiltered();
    (void)LightSensor_UpdateLevel(filt); /* mantenemos el nivel actualizado, pero no lo mostramos */

    uint8_t blink = 0;
    if (st == SETUP_WAIT_LOW)
      blink = (uint8_t)((now / 500U) & 1U); /* 500ms */
    else if (st == SETUP_WAIT_HIGH)
      blink = (uint8_t)((now / 150U) & 1U); /* 150ms */

    LedUI_SetLevel(blink);
  }

  if (!btn_clicked())
    return;

  /* CLICK detectado: feedback sonoro */
  Buzzer_Beep(SETUP_BEEP_MS);

  /* CLICK detectado: toma medición */
  uint16_t v = LDR_ReadFiltered();

  if (st == SETUP_WAIT_LOW)
  {
    int ok = LightSensor_CalibSetLow(v);
    if (ok == 0)
    {
      setup_printf("\r\n[OK] Umbral BAJO guardado: %u\r\n", v);
      LedUI_Event(LED_EVENT_LETTER);
      st = SETUP_WAIT_HIGH;

      setup_printf("Ahora iluminá el LDR más de cerca y apretá el botón para guardar el umbral ALTO\r\n\r\n");
    }
    else
    {
      setup_printf("\r\n[ERR] Umbral BAJO inválido: %u (debe ser < umbral alto)\r\n\r\n", v);
      LedUI_Event(LED_EVENT_ERROR);
    }
    return;
  }

  if (st == SETUP_WAIT_HIGH)
  {
    int ok = LightSensor_CalibSetHigh(v);
    if (ok == 0)
    {
      setup_printf("\r\n[OK] Umbral ALTO guardado: %u\r\n", v);
      LedUI_Event(LED_EVENT_LETTER);

      /*  Guarda en Flash para volver a usar */
      nv_cfg_t cfg;
      cfg.th_low  = LightSensor_GetThLow();
      cfg.th_high = LightSensor_GetThHigh();
      cfg.tu_ms   = (uint16_t)Morse_GetTu();

      int rc = NvStore_Save(&cfg);
      if (rc == 0)
        setup_printf("[FLASH] Config guardada en Flash\r\n");
      else
        setup_printf("[FLASH] ERROR al guardar config (rc=%d)\r\n", rc);

      setup_printf("\r\n[SETUP] Listo ✓\r\n");
      setup_printf("Umbral BAJO = %u\r\n", cfg.th_low);
      setup_printf("Umbral ALTO = %u\r\n", cfg.th_high);
      setup_printf("Tu         = %u ms\r\n", cfg.tu_ms);
      setup_printf("\r\nAhora usá:  mode light   (para decodificar con linterna)\r\n\r\n");

      st = SETUP_DONE;

      /* cuando termina, apagamos el blink */
      LedUI_SetLevel(0);
    }
    else
    {
      setup_printf("\r\n[ERR] Umbral ALTO inválido: %u (debe ser > umbral bajo)\r\n\r\n", v);
      LedUI_Event(LED_EVENT_ERROR);
    }
    return;
  }
}


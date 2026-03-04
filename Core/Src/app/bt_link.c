/*
 * bt_link.c
 *
 *  Created on: Jan 31, 2026
 *      Author: feder
 */

#include "app/bt_link.h"
#include <string.h>
#include <stdio.h>

static UART_HandleTypeDef *s_huart = NULL;
static uint8_t s_enabled = 0;

/* RX handling */
static uint8_t s_rx_byte = 0;
#define BT_RX_BUF_SZ 128
static volatile uint16_t s_rx_w = 0;
static volatile uint16_t s_rx_r = 0;
static uint8_t s_rx_buf[BT_RX_BUF_SZ];

static void (*s_on_byte)(uint8_t b) = NULL;

/* Filtro de salida para BT */
static char s_last_symbol[16] = {0};  /* ".-" etc */
static uint8_t s_last_symbol_valid = 0;

static void bt_tx_raw(const uint8_t *data, uint16_t len)
{
  if (!s_enabled || s_huart == NULL) return;
  if (data == NULL || len == 0) return;

  (void)HAL_UART_Transmit(s_huart, (uint8_t*)data, len, 200);
}

static void bt_maybe_emit_symbol_before_prompt(const uint8_t *data, uint16_t len)
{
  /* Si el siguiente texto va a imprimir ">> ..." (letra/espacio),
     primero emitimos una línea "SIMBOLO: <...>" una sola vez. */
  if (!s_last_symbol_valid) return;
  if (s_last_symbol[0] == '\0') { s_last_symbol_valid = 0; return; }

  if (len >= 3)
  {
    /* típicamente llega como "\r\n>> ..." */
    if ((data[0] == '\r' && data[1] == '\n' && data[2] == '>')
        || (data[0] == '\n' && data[1] == '>' ))
    {
      char line[32];
      int n = snprintf(line, sizeof(line), "SIMBOLO: %s\r\n", s_last_symbol);
      if (n > 0) bt_tx_raw((const uint8_t*)line, (uint16_t)n);
      s_last_symbol_valid = 0;
    }
  }
}

void BtLink_Init(UART_HandleTypeDef *huart)
{
  s_huart = huart;
  s_enabled = (huart != NULL) ? 1U : 0U;

  s_rx_w = s_rx_r = 0;
  s_last_symbol[0] = '\0';
  s_last_symbol_valid = 0;

}

void BtLink_SetEnabled(uint8_t en)
{
  s_enabled = (en != 0U) ? 1U : 0U;
}

uint8_t BtLink_IsEnabled(void)
{
  return (s_enabled && (s_huart != NULL)) ? 1U : 0U;
}

void BtLink_AttachCliRx(void (*on_byte)(uint8_t b))
{
  s_on_byte = on_byte;
}

uint8_t BtLink_OnUartRxCplt(UART_HandleTypeDef *huart)
{
  if (!BtLink_IsEnabled()) return 0;
  if (huart != s_huart) return 0;

  /* push a ring */
  uint16_t next = (uint16_t)((s_rx_w + 1U) % BT_RX_BUF_SZ);
  if (next != s_rx_r)
  {
    s_rx_buf[s_rx_w] = s_rx_byte;
    s_rx_w = next;
  }

  /* rearm RX */
  (void)HAL_UART_Receive_IT(s_huart, &s_rx_byte, 1);
  return 1;
}

void BtLink_Task(void)
{
  if (!BtLink_IsEnabled()) return;
  if (s_on_byte == NULL) return;

  while (s_rx_r != s_rx_w)
  {
    uint8_t b = s_rx_buf[s_rx_r];
    s_rx_r = (uint16_t)((s_rx_r + 1U) % BT_RX_BUF_SZ);
    s_on_byte(b);
  }
}

static uint8_t is_symbol_update(const uint8_t *data, uint16_t len)
{
  /* Detecta "\rSIMBOLO: " al inicio (salida de symdbg_show) */
  if (len < 10) return 0;
  if (data[0] != '\r') return 0;
  const char *p = (const char*)data;
  return (strncmp(p+1, "SIMBOLO:", 8) == 0) ? 1U : 0U;
}

static void cache_symbol_from_update(const uint8_t *data, uint16_t len)
{
  /* data: "\rSIMBOLO: <txt padded>" */
  const char *p = (const char*)data;
  const char *s = strstr(p, "SIMBOLO:");
  if (!s) return;
  s += 8;
  while (*s == ' ') s++;

  /* Copia hasta fin de buffer y trimea espacios */
  char tmp[16];
  size_t n = 0;
  while (n < sizeof(tmp)-1 && (s - p) < (int)len)
  {
    char c = *s++;
    if (c == '\0') break;
    tmp[n++] = c;
  }
  tmp[n] = '\0';

  /* trim right */
  while (n > 0 && (tmp[n-1] == ' ' || tmp[n-1] == '\r' || tmp[n-1] == '\n' || tmp[n-1] == '\t'))
  {
    tmp[n-1] = '\0';
    n--;
  }

  strncpy(s_last_symbol, tmp, sizeof(s_last_symbol)-1);
  s_last_symbol[sizeof(s_last_symbol)-1] = '\0';
  s_last_symbol_valid = 1;
}

void BtLink_Write(const uint8_t *data, uint16_t len)
{
  if (!BtLink_IsEnabled()) return;
  if (data == NULL || len == 0) return;

  /* captura/filtra updates de "SIMBOLO:" (con '\r' sin '\n') */
  if (is_symbol_update(data, len))
  {
    cache_symbol_from_update(data, len);
    return; /* no lo imprimimos tal cual en BT */
  }

  /* Si el siguiente texto es ">> ..." emitimos la línea de símbolo antes */
  bt_maybe_emit_symbol_before_prompt(data, len);

  /*  Passthrough con normalización mínima */
  uint8_t out[256];
  uint16_t w = 0;

  for (uint16_t i = 0; i < len && w < sizeof(out); i++)
  {
    uint8_t c = data[i];

    if (c == '\b') continue;

    if (c == '\r')
    {
      /* mantener \r\n, descartar \r solo */
      if (i + 1 < len && data[i+1] == '\n')
      {
        out[w++] = '\r';
      }
      else
      {
        continue;
      }
    }
    else
    {
      out[w++] = c;
    }
  }

  bt_tx_raw(out, w);
}

void BtLink_WriteStr(const char *s)
{
  if (s == NULL) return;
  BtLink_Write((const uint8_t*)s, (uint16_t)strlen(s));
}

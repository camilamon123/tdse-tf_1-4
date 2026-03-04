/*
 * oled_ui.c
 *
 *  Created on: Feb 10, 2026
 *      Author: feder
 */

#include "app/oled_ui.h"

#include <string.h>
#include <stdio.h>

#include "drivers/ssd1306.h"
#include "main.h"


/* Refresh */
#define OLED_REFRESH_MS       100u   /* 10 Hz */

/* =========================================================
 *  ESTADO
 * ========================================================= */
static SPI_HandleTypeDef *s_hspi = NULL;
static ssd1306_t s_disp;
static uint8_t s_inited = 0;

static uint32_t s_next_refresh = 0;

/* Snapshot */
typedef struct
{
  app_mode_t mode;
  uint8_t bt_connected;
  uint32_t tu_ms;
  uint16_t th_low;
  uint16_t th_high;
  uint8_t ldr_pct;
  char sym_dbg[17];
  char last_phrase[65];
} oled_snapshot_t;

static volatile oled_snapshot_t s_snap;

/* =========================================================
 *  HELPERS
 * ========================================================= */
static const char* mode_str(app_mode_t m)
{
  switch (m)
  {
    case MODE_NONE:       return "NONE";
    case MODE_MORSE:      return "MORSE";
    case MODE_LDR_SIM:    return "LDR";
    case MODE_LIGHT_MORSE:return "LIGHT";
    case MODE_SETUP_BTN:  return "SETUP";
    default:              return "UNK";
  }
}

/* Dibuja una línea (8px de alto) */
static void draw_line(uint8_t line, const char *text)
{
  if (line >= 8u) return;
  SSD1306_DrawString5x7At(&s_disp, 0u, (uint8_t)(line * 8u), text);
}

/* Imprime una frase “wrap” en 2 líneas (líneas 6 y 7) */
static void draw_wrapped_phrase_2lines(const char *phrase)
{
  /* max aprox 21 chars por línea (128/6=21) */
  char l1[23], l2[23];
  memset(l1, 0, sizeof(l1));
  memset(l2, 0, sizeof(l2));

  size_t n = strlen(phrase);
  if (n <= 21)
  {
    strncpy(l1, phrase, 21);
  }
  else
  {
    strncpy(l1, phrase, 21);
    strncpy(l2, phrase + 21, 21);
  }

  draw_line(6, l1);
  draw_line(7, l2);
}

/* =========================================================
 *  API
 * ========================================================= */
void OledUI_Init(SPI_HandleTypeDef *hspi)
{
  s_hspi = hspi;
  s_inited = 0;

  memset((void*)&s_snap, 0, sizeof(s_snap));
  strncpy((char*)s_snap.sym_dbg, "", sizeof(s_snap.sym_dbg) - 1);
  strncpy((char*)s_snap.last_phrase, "", sizeof(s_snap.last_phrase) - 1);

  /* s_inited se completa en Task, así no bloquea boot si algo falla */
}

void OledUI_SetSnapshot(app_mode_t mode,
                        uint8_t bt_connected,
                        uint32_t tu_ms,
                        uint16_t th_low,
                        uint16_t th_high,
                        uint8_t ldr_pct,
                        const char *sym_dbg,
                        const char *last_phrase)
{
  /* copia rápida  */
  oled_snapshot_t tmp;

  tmp.mode = mode;
  tmp.bt_connected = bt_connected ? 1u : 0u;
  tmp.tu_ms = tu_ms;
  tmp.th_low = th_low;
  tmp.th_high = th_high;
  tmp.ldr_pct = ldr_pct;

  memset(tmp.sym_dbg, 0, sizeof(tmp.sym_dbg));
  if (sym_dbg) strncpy(tmp.sym_dbg, sym_dbg, sizeof(tmp.sym_dbg) - 1);

  memset(tmp.last_phrase, 0, sizeof(tmp.last_phrase));
  if (last_phrase) strncpy(tmp.last_phrase, last_phrase, sizeof(tmp.last_phrase) - 1);

  __disable_irq();
  s_snap = tmp;
  __enable_irq();
}

/* Dibujo principal */
static void oled_draw(const oled_snapshot_t *sp)
{
  char linebuf[48];

  SSD1306_Clear(&s_disp);

  /* Línea 0: header */
  snprintf(linebuf, sizeof(linebuf), "M:%s  BT:%c",
           mode_str(sp->mode),
           sp->bt_connected ? 'Y' : 'N');
  draw_line(0, linebuf);

  /* Línea 1: thresholds */
  snprintf(linebuf, sizeof(linebuf), "ThL:%u ThH:%u",
           (unsigned)sp->th_low, (unsigned)sp->th_high);
  draw_line(1, linebuf);

  /* Línea 2: Tu */
  snprintf(linebuf, sizeof(linebuf), "Tu:%lums",
           (unsigned long)sp->tu_ms);
  draw_line(2, linebuf);

  /* Línea 3: LDR percent + bar */
  {
    /* bar 10 columnas */
    char bar[11];
    uint8_t filled = (uint8_t)((sp->ldr_pct + 9u) / 10u);
    if (filled > 10u) filled = 10u;
    for (uint8_t i = 0; i < 10u; i++)
      bar[i] = (i < filled) ? '#' : '-';
    bar[10] = '\0';

    snprintf(linebuf, sizeof(linebuf), "LDR:%3u%% %s",
             (unsigned)sp->ldr_pct, bar);
    draw_line(3, linebuf);
  }

  /* Línea 4: símbolo en construcción */
  snprintf(linebuf, sizeof(linebuf), "SIMBOLO:%s",
           sp->sym_dbg[0] ? sp->sym_dbg : "");
  draw_line(4, linebuf);

  /* Línea 5: separador / hint */
  draw_line(5, "LAST:");

  /* Líneas 6-7: última frase (wrap simple 2 líneas) */
  if (sp->last_phrase[0])
    draw_wrapped_phrase_2lines(sp->last_phrase);
  else
    draw_line(6, "(none)");

  SSD1306_Update(&s_disp);
}

void OledUI_Task(uint32_t now_ms)
{
  if (s_hspi == NULL) return;

  /* Init lazy */
  if (!s_inited)
  {
    if (SSD1306_Init(&s_disp,
                     s_hspi,
                     OLED_CS_GPIO_Port,  OLED_CS_Pin,
                     OLED_DC_GPIO_Port,  OLED_DC_Pin,
                     OLED_RES_GPIO_Port, OLED_RES_Pin,
                     0) == HAL_OK)
    {
      s_inited = 1u;
      s_next_refresh = now_ms; /* dibuja ya */
    }
    else
    {
      /* si falla, reintenta cada 500ms */
      s_next_refresh = now_ms + 500u;
      return;
    }
  }

  if ((uint32_t)(now_ms - s_next_refresh) < OLED_REFRESH_MS)
    return;

  s_next_refresh = now_ms;

  /* Snapshot local */
  oled_snapshot_t local;
  __disable_irq();
  local = s_snap;
  __enable_irq();

  oled_draw(&local);
}

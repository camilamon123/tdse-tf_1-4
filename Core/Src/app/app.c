/*
 * app.c
 *
 * Created on: Jan 22, 2026
 * Author: feder
 */

#include "app/app.h"
#include "main.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "app/button_input.h"
#include "app/morse_decoder.h"
#include "app/light_sensor.h"
#include "drivers/ldr.h"

#include "app/buzzer.h"
#include "app/led_ui.h"
#include "app/setup_btn.h"
#include "app/nv_store.h"
#include "app/bt_link.h"
#include "app/oled_ui.h"


/*
 * lo más importante del  proyecto.
 *
 * - Implementa el "modo" actual (morse por botón, monitor LDR, morse por luz, setup).
 * - Mide tiempos entre flancos (ON/OFF) para alimentar al decodificador Morse (morse_decoder.c).
 * - Maneja el buffer de frase y la impresión por UART (PuTTY) y Bluetooth (Serial Bluetooth Terminal) y OLED.
 * - Da feedback embebido: LED externo (LedUI_*) y buzzer (Buzzer_*), ambos no bloqueantes.
 * - Implementa un mini CLI por UART para comandos:
 * help, mode, status, ldr on/off/rate, tu, setup, flash clear, etc.
 *
 * que las tareas sean "no bloqueantes" es que se ejecutan en el superloop.
 */

/* ===================== MEDICIONES TI / WCET ===================== */
static uint32_t et_val = 0;           /* Execution Time actual (us) */
static uint32_t wcet_val = 0;         /* Worst Case Execution Time (us) */
static uint32_t t_last_report = 0;
static app_mode_t last_mode_tracked = MODE_NONE;

/* ===================== CONFIG ===================== */
#define DEFAULT_TU_MS 500U

#define SYMDBG_MAX    16
#define PHRASE_MAX    64

/* Fin de letra: valor entre 1.5Tu y 4.5Tu */
#define LETTER_FLUSH_MULT   3U

/* Espacio por “turno siguiente” */
#define SPACE_TURN_MULT     6U   // Tu=500ms => 3s

/* Fin de frase global (inactividad total) */
#define PHRASE_IDLE_MULT    14U  // Tu=500ms => 7s

/* CLI UART */
#define CLI_LINE_MAX 128

/* Para leer el ADC sin matarlo a 1kHz */
#define LDR_SAMPLE_MS  5U

/* Buzzer: duración del beep por letra (milisegundos) */
#define BUZZER_BEEP_MS  70U

/* ===================== UART PRINT ===================== */
static UART_HandleTypeDef *g_huart = NULL;

enum { OUT_UART = 1u, OUT_BT = 2u, OUT_BOTH = (OUT_UART | OUT_BT) };

static void uart_tx_mask(uint8_t mask, const uint8_t *data, uint16_t len)
{
  if (data == NULL || len == 0) return;

  if ((mask & OUT_UART) && (g_huart != NULL))
    HAL_UART_Transmit(g_huart, (uint8_t*)data, len, 100);

  if (mask & OUT_BT)
    BtLink_Write(data, len);
}

static void uart_tx_raw(const uint8_t *data, uint16_t len)
{
  uart_tx_mask(OUT_BOTH, data, len);
}

static void uart_vprintf_mask(uint8_t mask, const char *fmt, va_list args)
{
  char buf[260];
  int n = vsnprintf(buf, sizeof(buf), fmt, args);
  if (n < 0) return;
  if (n > (int)(sizeof(buf)-1)) n = (int)(sizeof(buf)-1);
  uart_tx_mask(mask, (const uint8_t*)buf, (uint16_t)n);
}

static void uart_printf_mask(uint8_t mask, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  uart_vprintf_mask(mask, fmt, args);
  va_end(args);
}

static void uart_printf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  uart_vprintf_mask(OUT_BOTH, fmt, args);
  va_end(args);
}

/* ===================== ESTADO GLOBAL ===================== */

static uint8_t  s_uart_banner_pending = 0;
static uint32_t s_uart_banner_due = 0;

#define UART_BANNER_DELAY_MS 1500u

static app_mode_t g_mode = MODE_NONE;

/* Morse timing + captura */
static uint32_t t_last_edge = 0;
static uint8_t  prev_level  = 0;

/* Para evitar un pulso “fantasma” al cambiar de modo */
static uint8_t ignore_next_edge = 0;
static uint32_t t_mode_sync = 0;

static uint8_t started = 0;
static uint8_t letter_timeout_sent = 0;

/* Debug símbolo actual */
static char sym_dbg[SYMDBG_MAX];
static uint8_t sym_len = 0;

/* Buffer frase completa */
static char phrase[PHRASE_MAX];
static uint8_t phrase_len = 0;

/* Última frase finalizada (se conserva para STATUS) */
static char ultima_frase[PHRASE_MAX];
static uint8_t ultima_frase_ok = 0;

/* Espacio por “turno siguiente” */
static uint8_t  space_armed = 0;
static uint32_t t_space_arm = 0;
static uint8_t  word_has_letters = 0;

/* actividad real para fin de frase global */
static uint32_t t_last_activity = 0;

/* CLI RX */
static char cli_line[CLI_LINE_MAX];
static volatile uint16_t cli_len = 0;
static volatile uint8_t cli_ready = 0;

/* timers no bloqueantes para otros modos */
static uint32_t t_last_ldr_print = 0;

/* cache ADC para modo light */
static uint32_t t_last_ldr_sample = 0;

/* ---- LDR monitor (solo modo LDR) ---- */
static uint32_t t_last_ldr_sample_mon = 0;
#define LDR_MON_SAMPLE_MS  5U

static uint8_t  ldr_stream_on = 1;     /* estos dos solo afectan */
static uint32_t ldr_rate_ms   = 200U;  /* al modo ldr */

static uint16_t ldr_last_filt  = 0;
static uint16_t ldr_last_raw   = 0;
static uint8_t  ldr_last_pct   = 0;
static uint8_t  ldr_last_level = 0;

static nv_cfg_t s_cfg;
/* Estado BT (HC-05 STATE) con debounce */
static uint8_t  s_bt_raw = 0;                 /* lectura instantánea */
static uint8_t  s_bt_stable = 0;              /* estado debounced */
static uint32_t s_bt_last_raw_change_ms = 0;  /* tick del último cambio de raw */

static uint8_t  s_bt_connected = 0;           /* espejo del estado estable */

static uint8_t  s_bt_banner_pending = 0;
static uint32_t s_bt_banner_due = 0;
static uint32_t s_bt_last_banner_ms = 0;
static uint32_t s_bt_poll_due = 0;

#define BT_POLL_MS            10u
/* Debounce del pin STATE del HC-05.
  Si el pin rebota al conectar/reconectar, el banner podía cortarse (BT enable
  bajaba a mitad de transmisión) o imprimirse más de una vez.
 */
#define BT_DEBOUNCE_MS        60u
/* Espera extra luego de detectar conexión estable antes de mandar el banner. */
#define BT_BANNER_DELAY_MS    500u
/* Anti-spam por las dudas (rebotes largos / app que conecta y desconecta rápido). */
#define BT_BANNER_MIN_GAP_MS  2000u

/* ===================== DEBUG SYM ===================== */
static void symdbg_reset(void)
{
  sym_len = 0;
  sym_dbg[0] = '\0';
}

static void symdbg_push(char s)
{
  if (sym_len < (SYMDBG_MAX - 1))
  {
    sym_dbg[sym_len++] = s;
    sym_dbg[sym_len] = '\0';
  }
}

static void symdbg_show(void)
{
  uart_printf("\rSIMBOLO: %-12s", sym_dbg);
}

/* ===================== PHRASE ===================== */
static void phrase_reset(void)
{
  phrase_len = 0;
  phrase[0] = '\0';

  started = 0;
  letter_timeout_sent = 0;

  space_armed = 0;
  word_has_letters = 0;

  symdbg_reset();
}

static void phrase_append(char c)
{
  if (c == ' ')
  {
    if (phrase_len == 0) return;
    if (phrase[phrase_len - 1] == ' ') return;
  }

  if (phrase_len < (PHRASE_MAX - 1))
  {
    phrase[phrase_len++] = c;
    phrase[phrase_len] = '\0';
  }
}

static void print_char_immediate(char c)
{
  if (c == ' ')
    uart_printf("\r\n>> [espacio]\r\n");
  else
    uart_printf("\r\n>> %c\r\n", c);

}

/* ===================== SYNC INPUT LEVEL SEGÚN MODO ===================== */
static void App_SyncPrevLevel(uint32_t now)
{
  if (g_mode == MODE_MORSE)
  {
	ButtonInput_Init();
    prev_level = ButtonInput_GetStable();
  }
  else if (g_mode == MODE_LIGHT_MORSE)
  {
    uint16_t filt = LDR_ReadFiltered();
    LightSensor_SyncLevel(filt);
    prev_level = LightSensor_UpdateLevel(filt);
  }
  else
  {
    prev_level = 0;
  }

  t_last_edge = now;
  t_last_activity = now;

  /*
   * ignora el flanco basura.
   */
  ignore_next_edge = 1;
  t_mode_sync = now;
}

/* ===================== FIN DE FRASE POR IDLE ===================== */
static void phrase_try_print_on_idle(uint32_t now, uint8_t level)
{
  if (phrase_len == 0 && sym_len == 0 && !started) return;

  uint32_t Tu = Morse_GetTu();
  uint32_t idle_ms = PHRASE_IDLE_MULT * Tu;
  uint32_t dt_idle = now - t_last_activity;

  if (dt_idle < idle_ms) return;

  if (started && level == 0)
  {
    uint32_t t_letter = LETTER_FLUSH_MULT * Tu;
    Morse_OffGap(t_letter);
  }

  char c;
  while (Morse_PopChar(&c))
  {
    phrase_append(c);
    print_char_immediate(c);
    symdbg_reset();
  }

  if (phrase_len > 0)
  {
    strncpy(ultima_frase, phrase, PHRASE_MAX);
    ultima_frase[PHRASE_MAX - 1] = '\0';
    ultima_frase_ok = 1;

    uart_printf("\r\nFRASE: %s\r\n\r\n", phrase);
  }

  phrase_reset();

}

/* ===================== CLI ===================== */
static void CLI_PrintHelp(void);

static void App_ModeBanner(void)
{
  if (g_mode == MODE_MORSE)
  {
    uart_printf("\r\n[MORSE] Botón -> decodifica\r\n\r\n");
  }
  else if (g_mode == MODE_LDR_SIM)
  {
    uart_printf("\r\n[LDR] ADC + %% + nivel + bargraph\r\n");
    uart_printf("Usá: ldr on | ldr off | ldr rate <ms>\r\n\r\n");
  }
  else if (g_mode == MODE_LIGHT_MORSE)
  {
    uart_printf("\r\n[LUZ] Linterna -> decodifica\r\n");
    uart_printf("Tip: antes usá 'mode setup' para calibrar el umbral si no lo hiciste\r\n\r\n");
  }
}

static void CLI_HandleLine(char *line)
{
  for (int i = 0; line[i]; i++)
  {
    if (line[i] == '\r' || line[i] == '\n')
    {
      line[i] = '\0';
      break;
    }
  }

  if (strcmp(line, "help") == 0)
  {
    CLI_PrintHelp();
    return;
  }

  if (strcmp(line, "status") == 0)
  {
    uart_printf("\r\nESTADO:\r\n");

    uart_printf("Modo = ");
    if (g_mode == MODE_NONE) uart_printf("(ninguno)\r\n");
    else if (g_mode == MODE_MORSE) uart_printf("morse\r\n");
    else if (g_mode == MODE_LDR_SIM) uart_printf("ldr\r\n");
    else if (g_mode == MODE_LIGHT_MORSE) uart_printf("light\r\n");
    else if (g_mode == MODE_SETUP_BTN) uart_printf("setup\r\n");
    else uart_printf("(ninguno)\r\n");

    uart_printf("Tu = %lu ms\r\n", (unsigned long)Morse_GetTu());

    uart_printf("Umbral bajo = %u\r\n", LightSensor_GetThLow());
    uart_printf("Umbral alto = %u\r\n", LightSensor_GetThHigh());
    uart_printf("Nivel = %u\r\n", LightSensor_GetLevel());

    uart_printf("Ultima frase = '%s'\r\n",
                (ultima_frase_ok && ultima_frase[0] != '\0') ? ultima_frase : "(ninguna)");

    if (g_mode == MODE_LDR_SIM)
    {
      uart_printf("LDR stream = %s\r\n", ldr_stream_on ? "on" : "off");
      uart_printf("LDR rate   = %lu ms\r\n", (unsigned long)ldr_rate_ms);
    }

    uart_printf("\r\n");
    return;
  }

  if (strcmp(line, "clear") == 0)
  {
    phrase_reset();
    symdbg_reset();

    ultima_frase[0] = '\0';
    ultima_frase_ok = 0;

    uart_printf("\r\n[OK] frase limpiada\r\n");
    return;
  }

  if (strcmp(line, "flash clear") == 0)
  {
    if (NvStore_Clear() == 0)
      uart_printf("\r\n[OK] Flash borrada (config reset)\r\n");
    else
      uart_printf("\r\n[ERR] No se pudo borrar flash\r\n");
    return;
  }

  if (strncmp(line, "tu ", 3) == 0)
  {
    uint32_t new_tu = (uint32_t)strtoul(&line[3], NULL, 10);

    if (new_tu < 80 || new_tu > 2000)
    {
      uart_printf("\r\n[ERR] tu fuera de rango (80..2000)\r\n");
      return;
    }

    Morse_Init(new_tu);

    uint32_t now = HAL_GetTick();
    phrase_reset();
    symdbg_reset();
    App_SyncPrevLevel(now);

    uart_printf("\r\n[OK] Tu actualizado a %lu ms\r\n", (unsigned long)new_tu);
    return;
  }

  /* ====== comandos LDR (SOLO para modo LDR) ====== */
  if (strcmp(line, "ldr on") == 0)
  {
    if (g_mode != MODE_LDR_SIM)
    {
      uart_printf("\r\n[ERR] 'ldr on' solo en modo ldr (usá: mode ldr)\r\n");
      return;
    }
    ldr_stream_on = 1;
    uart_printf("\r\n[OK] ldr stream ON\r\n");
    return;
  }

  if (strcmp(line, "ldr off") == 0)
  {
    if (g_mode != MODE_LDR_SIM)
    {
      uart_printf("\r\n[ERR] 'ldr off' solo en modo ldr (usá: mode ldr)\r\n");
      return;
    }
    ldr_stream_on = 0;
    uart_printf("\r\n[OK] ldr stream OFF\r\n");
    return;
  }

  if (strncmp(line, "ldr rate ", 9) == 0)
  {
    if (g_mode != MODE_LDR_SIM)
    {
      uart_printf("\r\n[ERR] 'ldr rate' solo en modo ldr (usá: mode ldr)\r\n");
      return;
    }

    uint32_t ms = (uint32_t)strtoul(&line[9], NULL, 10);
    if (ms < 50U || ms > 5000U)
    {
      uart_printf("\r\n[ERR] rate fuera de rango (50..5000 ms)\r\n");
      return;
    }

    ldr_rate_ms = ms;
    uart_printf("\r\n[OK] ldr rate = %lu ms\r\n", (unsigned long)ldr_rate_ms);
    return;
  }

  if (strcmp(line, "ldr") == 0 || strcmp(line, "ldr show") == 0)
  {
    if (g_mode != MODE_LDR_SIM)
    {
      uart_printf("\r\n[ERR] 'ldr' solo en modo ldr (usá: mode ldr)\r\n");
      return;
    }
    uart_printf("\r\n[LDR]\r\n");
    uart_printf("  stream = %s\r\n", ldr_stream_on ? "on" : "off");
    uart_printf("  rate   = %lu ms\r\n\r\n", (unsigned long)ldr_rate_ms);
    return;
  }

  if (strcmp(line, "mode") == 0)
  {
    uart_printf("\r\nMODO: ");
    if (g_mode == MODE_NONE) uart_printf("(ninguno)\r\n\r\n");
    else if (g_mode == MODE_MORSE) uart_printf("morse\r\n\r\n");
    else if (g_mode == MODE_LDR_SIM) uart_printf("ldr\r\n\r\n");
    else if (g_mode == MODE_CALIB_SIM) uart_printf("calib\r\n\r\n");
    else if (g_mode == MODE_LIGHT_MORSE) uart_printf("light\r\n\r\n");
    else if (g_mode == MODE_SETUP_BTN) uart_printf("setup\r\n\r\n");
    else uart_printf("(ninguno)\r\n\r\n");
    return;
  }

  if (strncmp(line, "mode ", 5) == 0)
  {
    char *m = &line[5];

    if (strcmp(m, "morse") == 0) g_mode = MODE_MORSE;
    else if (strcmp(m, "ldr") == 0) g_mode = MODE_LDR_SIM;
    else if (strcmp(m, "light") == 0) g_mode = MODE_LIGHT_MORSE;
    else if (strcmp(m, "setup") == 0) g_mode = MODE_SETUP_BTN;
    else
    {
      uart_printf("\r\n[ERR] modo desconocido. Usá: morse | ldr | light | setup\r\n\r\n");
      return;
    }

    uint32_t now = HAL_GetTick();
    phrase_reset();
    symdbg_reset();
    App_SyncPrevLevel(now);

    uart_printf("\r\n[OK] modo cambiado\r\n");

    if (g_mode == MODE_SETUP_BTN)
      SetupBtn_Start(now);

    App_ModeBanner();
    return;
  }

  uart_printf("\r\n[ERR] comando desconocido: '%s'\r\n", line);
  uart_printf("Escribí 'help'\r\n");
}

static void CLI_PrintHelp(void)
{
  uart_printf("\r\nComandos:\r\n");
  uart_printf("  help           -> muestra esta ayuda\r\n");
  uart_printf("  status         -> muestra el estado actual\r\n");
  uart_printf("  tu <ms>        -> setea Tu en ms (80..2000)\r\n");
  uart_printf("  clear          -> borra la frase\r\n");
  uart_printf("  mode           -> muestra el modo actual\r\n");
  uart_printf("  mode <nombre> -> morse | ldr | light | setup\r\n\r\n");

  uart_printf("  flash clear   -> borra config guardada en Flash\r\n\r\n");

  uart_printf("  (solo en mode ldr)\r\n");
  uart_printf("    ldr on        -> habilita impresión periódica\r\n");
  uart_printf("    ldr off       -> deshabilita impresión periódica\r\n");
  uart_printf("    ldr rate <ms> -> periodo impresión (50..5000)\r\n");
  uart_printf("    ldr           -> muestra config ldr\r\n");
}

static void CLI_Task(void)
{
  if (!cli_ready) return;

  char line_copy[CLI_LINE_MAX];

  __disable_irq();
  strncpy(line_copy, cli_line, CLI_LINE_MAX);
  cli_ready = 0;
  cli_len = 0;
  memset(cli_line, 0, sizeof(cli_line));
  __enable_irq();

  char *p = line_copy;
  while (*p == ' ' || *p == '\t') p++;

  if (*p == '\0') return;

  CLI_HandleLine(p);
}

/* App recibe bytes de UART (desde callback) */
void App_OnUartRxByte(uint8_t b)
{
  char c = (char)b;

  /* Ignorar secuencias ESC (flechitas, etc.) */
  static uint8_t esc_state = 0;

  if (esc_state)
  {
    if (esc_state == 1) { esc_state = (c == '[') ? 2 : 0; return; }
    if (esc_state == 2) { esc_state = 0; return; }
  }

  if ((uint8_t)c == 0x1B) { esc_state = 1; return; } // ESC

  /* ENTER: CR o LF (si viene CRLF, el LF se ignora) */
  static uint8_t last_was_cr = 0;

  if (c == '\r' || (c == '\n' && !last_was_cr))
  {
    last_was_cr = (c == '\r') ? 1u : 0u;
    uart_printf("\r\n");
    if (cli_len > 0)
    {
      cli_line[cli_len] = '\0';
      cli_ready = 1;
    }
    return;
  }

  if (c != '\n') last_was_cr = 0;

  /* Backspace (BS o DEL) */
  if (c == '\b' || (uint8_t)c == 127)
  {
    if (cli_len > 0)
    {
      cli_len--;
      uart_printf("\b \b");
    }
    return;
  }

  /* CTRL+U borra toda la línea */
  if ((uint8_t)c == 0x15)
  {
    while (cli_len > 0)
    {
      cli_len--;
      uart_printf("\b \b");
    }
    return;
  }

  /* caracteres imprimibles */
  if (c >= 32 && c <= 126)
  {
    if (cli_len < (CLI_LINE_MAX - 1))
    {
      cli_line[cli_len++] = c;
      uart_tx_raw((const uint8_t*)&c, 1);
}
    else
    {
      cli_len = 0;
      memset(cli_line, 0, sizeof(cli_line));
      uart_printf("\r\n[ERR] línea demasiado larga\r\n");
    }
  }
}

/* ===================== DECODER ===================== */
static void App_Task_MorseFromLevel(uint32_t now, uint8_t level)
{
  if (level != prev_level)
  {
    if (ignore_next_edge)
    {
      uint32_t Tu_tmp = Morse_GetTu();
      uint32_t ignore_ms = Tu_tmp / 4U;   // ~0.25Tu
      if (ignore_ms < 20U)  ignore_ms = 20U;
      if (ignore_ms > 150U) ignore_ms = 150U;

      if ((now - t_mode_sync) <= ignore_ms)
      {
        prev_level = level;
        t_last_edge = now;
        t_last_activity = now;
        letter_timeout_sent = 0;
        ignore_next_edge = 0;
        return;
      }

      ignore_next_edge = 0;
    }

    uint32_t dt = now - t_last_edge;
    uint32_t Tu = Morse_GetTu();

    if (prev_level == 1)
    {
      uint32_t min_on = Tu / 4U;   // 0.25Tu
      if (min_on < 10U) min_on = 10U;

      if (dt >= min_on)
      {
        Morse_OnPulse(dt);

        started = 1;
        symdbg_push((dt < (2U * Tu)) ? '.' : '-');
        symdbg_show();
      }
    }
    else
    {
      space_armed = 0;
    }

    prev_level = level;
    t_last_edge = now;
    t_last_activity = now;
    letter_timeout_sent = 0;
  }

  if (level == 0)
  {
    uint32_t Tu = Morse_GetTu();
    uint32_t dt_off = now - t_last_edge;

    if (started && !letter_timeout_sent && dt_off >= (LETTER_FLUSH_MULT * Tu))
    {
      Morse_OffGap(LETTER_FLUSH_MULT * Tu);
      letter_timeout_sent = 1;
    }

    if (space_armed)
    {
      uint32_t dt_space = now - t_space_arm;
      if (dt_space >= (SPACE_TURN_MULT * Tu))
      {
        if (word_has_letters)
        {
          phrase_append(' ');
          print_char_immediate(' ');
          LedUI_Event(LED_EVENT_SPACE);
          t_last_activity = now;
        }
        space_armed = 0;
        word_has_letters = 0;
      }
    }
  }

  char c;
  while (Morse_PopChar(&c))
  {
    phrase_append(c);
    print_char_immediate(c);

    if (c == ' ')
      LedUI_Event(LED_EVENT_SPACE);
    else if (c == '?')
      LedUI_Event(LED_EVENT_ERROR);
    else
      LedUI_Event(LED_EVENT_LETTER);

    if (c != ' ')
      Buzzer_Beep(BUZZER_BEEP_MS);

    if (c != ' ')
    {
      word_has_letters = 1;
      space_armed = 1;
      t_space_arm = now;
    }

    symdbg_reset();

    started = 0;
    letter_timeout_sent = 0;
    t_last_activity = now;
  }

  phrase_try_print_on_idle(now, level);
}

/* ===================== TASKS POR MODO ===================== */
static void App_Task_Morse(uint32_t now)
{
  uint8_t level = ButtonInput_GetStable();
  App_Task_MorseFromLevel(now, level);
}

static void App_Task_LdrMonitor(uint32_t now)
{
  if ((now - t_last_ldr_sample_mon) >= LDR_MON_SAMPLE_MS)
  {
    t_last_ldr_sample_mon = now;

    ldr_last_filt  = LDR_ReadFiltered();
    ldr_last_raw   = LDR_GetLastRaw();
    ldr_last_pct   = LightSensor_ComputePercent(ldr_last_filt);
    ldr_last_level = LightSensor_UpdateLevel(ldr_last_filt);

    /* LED refleja nivel en modo LDR */
    LedUI_SetLevel(ldr_last_level);
  }

  /* impresión “spam” controlada SOLO para el comando ldr */
  if (!ldr_stream_on) return;
  if ((now - t_last_ldr_print) < ldr_rate_ms) return;
  t_last_ldr_print = now;

  /* bargraph 10 columnas */
  char bar[11];
  uint8_t filled = (uint8_t)((ldr_last_pct + 9U) / 10U);  // redondeo
  if (filled > 10U) filled = 10U;
  for (uint8_t i = 0; i < 10U; i++)
    bar[i] = (i < filled) ? '#' : '-';
  bar[10] = '\0';

  uart_printf("\r\nLDR: %s (%u%%)  bruto=%u  filt=%u  nivel=%u  thL=%u  thH=%u\r\n",
              bar,
              ldr_last_pct,
              ldr_last_raw,
              ldr_last_filt,
              ldr_last_level,
              LightSensor_GetThLow(),
              LightSensor_GetThHigh());
}

static void App_Task_LightMorse(uint32_t now)
{
  if ((now - t_last_ldr_sample) >= LDR_SAMPLE_MS)
  {
    t_last_ldr_sample = now;
    uint16_t filt = LDR_ReadFiltered();
    uint8_t  lvl  = LightSensor_UpdateLevel(filt);

    /* para UI (OLED / monitoreo) */
    ldr_last_raw   = LDR_GetLastRaw();
    ldr_last_filt  = filt;
    ldr_last_pct   = LightSensor_ComputePercent(filt);
    ldr_last_level = lvl;
  }

  uint8_t level = LightSensor_GetLevel();
  LedUI_SetLevel(level);
  App_Task_MorseFromLevel(now, level);
}

/* ===================== API ===================== */

static void App_PrintBanner(uint8_t outmask)
{
  uint32_t tu  = Morse_GetTu();
  uint16_t thL = LightSensor_GetThLow();
  uint16_t thH = LightSensor_GetThHigh();

  uart_printf_mask(outmask,
    "\r\n[UART OK] - [BT OK]\r\n\r\n"
    "Bienvenido a la decodificación morse mediante luz o botón.\r\n\r\n"
    "Config actual: bajo=%u alto=%u Tu=%lu ms\r\n\r\n"
    "Para más información escriba 'help' y ENTER.\r\n"
    "> ",
    (unsigned)thL,
    (unsigned)thH,
    (unsigned long)tu
  );
}

static inline uint8_t BtState_Read(void)
{
  return (HAL_GPIO_ReadPin(BT_STATE_GPIO_Port, BT_STATE_Pin) == GPIO_PIN_SET) ? 1u : 0u;
}

static void BtState_Init(uint32_t now)
{
  s_bt_raw = BtState_Read();
  s_bt_stable = s_bt_raw;
  s_bt_last_raw_change_ms = now;

  s_bt_connected = s_bt_stable;

  /* Habilitá/Deshabilitá el TX por BT según haya link real */
  BtLink_SetEnabled(s_bt_connected ? 1u : 0u);

  s_bt_poll_due = now + BT_POLL_MS;
  s_bt_banner_pending = 0u;

  s_bt_last_banner_ms = now - BT_BANNER_MIN_GAP_MS;

  /* Si ya estaba conectado al boot, mandamos banner por BT (con delay mayor) */
  if (s_bt_connected)
  {
    s_bt_banner_pending = 1u;
    s_bt_banner_due = now + BT_BANNER_DELAY_MS;
  }
}

static void BtState_Task(uint32_t now)
{
  if (now < s_bt_poll_due) return;
  s_bt_poll_due = now + BT_POLL_MS;

  /* lectura instantánea */
  uint8_t v = BtState_Read();
  if (v != s_bt_raw)
  {
    s_bt_raw = v;
    s_bt_last_raw_change_ms = now;
  }

  /* cambio de estado estable (debounce) */
  if (s_bt_raw != s_bt_stable)
  {
    if ((uint32_t)(now - s_bt_last_raw_change_ms) >= BT_DEBOUNCE_MS)
    {
      s_bt_stable = s_bt_raw;
      s_bt_connected = s_bt_stable;

      BtLink_SetEnabled(s_bt_connected ? 1u : 0u);

      if (s_bt_connected)
      {
        /* conectó: programar banner */
        s_bt_banner_pending = 1u;
        s_bt_banner_due = now + BT_BANNER_DELAY_MS;
      }
      else
      {
        /* desconectó: cancelar banner pendiente */
        s_bt_banner_pending = 0u;
      }
    }
  }

  /* Enviar banner solo por BT cuando ya está conectado y pasó el delay */
  if (s_bt_banner_pending && s_bt_connected && (now >= s_bt_banner_due))
  {
    /* si todavía no pasó el gap, NO canceles... reintenta en el próximo poll */
    if ((uint32_t)(now - s_bt_last_banner_ms) < BT_BANNER_MIN_GAP_MS)
      return;

    App_PrintBanner(OUT_BT); /* SOLO BT */
    s_bt_last_banner_ms = now;
    s_bt_banner_pending = 0u;
  }
}


void App_Init(UART_HandleTypeDef *huart)
{
  g_huart = huart;

  /* feedback embebido */
  LedUI_Init(LED_EXT_GPIO_Port, LED_EXT_Pin);
  Buzzer_Init(BUZZER_GPIO_Port, BUZZER_Pin);

  /* setup necesita UART para imprimir */
  SetupBtn_Init(huart);

  ButtonInput_Init();
  LDR_Init();

  /* Defaults + carga de configuración previa (si no hay, quedan defaults). */
  s_cfg.th_low  = 1200;
  s_cfg.th_high = 1800;
  s_cfg.tu_ms   = DEFAULT_TU_MS;

  /* Carga configuración previa (si no hay, queda en defaults). */
  (void)NvStore_Load(&s_cfg);

  Morse_Init(s_cfg.tu_ms);
  LightSensor_Init(s_cfg.th_low, s_cfg.th_high);

  static uint8_t s_uart_banner_sent = 0;
  uint32_t now = HAL_GetTick();

  if (!s_uart_banner_sent)
  {
    s_uart_banner_pending = 1u;
    s_uart_banner_due = now + UART_BANNER_DELAY_MS;
    s_uart_banner_sent = 1u;
  }


  /* init detector de conexión BT (para banner al conectar) */
  BtState_Init(now);

  phrase_reset();

  ultima_frase[0] = '\0';
  ultima_frase_ok = 0;

  symdbg_reset();
  App_SyncPrevLevel(now);
}

void App_Task(uint32_t now)
{
  CLI_Task();

  /* RX por BT lo maneja main.c (UART1 RX IT + cola + App_OnUartRxByte).
      Dejamos BtLink sólo como TX/estado, para evitar dobles caminos. */
  BtState_Task(now);

  if (s_uart_banner_pending && (now >= s_uart_banner_due))
  {
    App_PrintBanner(OUT_UART);
    s_uart_banner_pending = 0u;
  }

  /* tareas no bloqueantes */
  Buzzer_Task(now);
  LedUI_Task(now);

  /* --- 1. Reset WCET si cambio de modo --- */
  if (g_mode != last_mode_tracked) {
      wcet_val = 0;
      et_val = 0;
      last_mode_tracked = g_mode;
      uart_printf("\r\n[PERF] Cambio de modo. Reset WCET.\r\n");
  }

  /* --- 2. Inicio de medición (Ti) --- */
  uint32_t ti_ticks = SysTick->VAL;

  if (g_mode == MODE_MORSE)
    App_Task_Morse(now);
  else if (g_mode == MODE_LDR_SIM)
    App_Task_LdrMonitor(now);
  else if (g_mode == MODE_LIGHT_MORSE)
    App_Task_LightMorse(now);
  else if (g_mode == MODE_SETUP_BTN)
    SetupBtn_Task(now);
  else
  {
    /* (no se usa) */
  }

  /* --- 3. Fin de medición (Tf) --- */
  uint32_t tf_ticks = SysTick->VAL;
  uint32_t diff_ticks = (ti_ticks > tf_ticks) ? (ti_ticks - tf_ticks) : (SysTick->LOAD - tf_ticks + ti_ticks);

  et_val = diff_ticks / 72; // Microsegundos (para 72MHz)

  if (et_val > wcet_val) {
      wcet_val = et_val;
  }

  /* --- 4. Sondeo cada 3 segundos --- */
  if (now - t_last_report >= 3000) {
      t_last_report = now;
      if (g_mode != MODE_NONE) {
          uart_printf("\r\n[TIEMPOS MODO %d] Ejecución actual (ET): %lu us | Peor caso (WCET): %lu us\r\n", (int)g_mode, et_val, wcet_val);
      }
  }

  OledUI_SetSnapshot(
    g_mode,
    s_bt_connected,
    Morse_GetTu(),
    LightSensor_GetThLow(),
    LightSensor_GetThHigh(),
    ldr_last_pct,
    sym_dbg,
    (ultima_frase_ok ? ultima_frase : "")
  );
}


/* ====== GETTERS para UI (OLED / debug) ====== */
app_mode_t App_GetMode(void)
{
  return g_mode;
}

uint32_t App_GetTuMs(void)
{
  return Morse_GetTu();
}

const char* App_GetSymbolDbg(void)
{
  return sym_dbg;
}

const char* App_GetLastPhrase(void)
{
  return (ultima_frase_ok && ultima_frase[0] != '\0') ? ultima_frase : "";
}

uint8_t App_GetBtConnected(void)
{
  return s_bt_connected ? 1u : 0u;
}

uint8_t App_GetLdrPct(void)
{
  return ldr_last_pct;
}

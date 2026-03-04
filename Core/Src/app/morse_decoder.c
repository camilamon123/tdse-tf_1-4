/*
 * morse_decoder.c
 *
 *  Created on: Jan 21, 2026
 *      Author: feder
 */

#include "app/morse_decoder.h"
#include <string.h>

typedef struct {
    const char *code;
    char ch;
} morse_map_t;

static const morse_map_t table[] = {
    // letras
    {".-", 'A'},   {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'},  {".", 'E'},
    {"..-.", 'F'}, {"--.", 'G'},  {"....", 'H'}, {"..", 'I'},   {".---", 'J'},
    {"-.-", 'K'},  {".-..", 'L'}, {"--", 'M'},   {"-.", 'N'},   {"---", 'O'},
    {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'},  {"...", 'S'},  {"-", 'T'},
    {"..-", 'U'},  {"...-", 'V'}, {".--", 'W'},  {"-..-", 'X'}, {"-.--", 'Y'},
    {"--..", 'Z'},

    // numeros
    {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'}, {"....-", '4'},
    {".....", '5'}, {"-....", '6'}, {"--...", '7'}, {"---..", '8'}, {"----.", '9'},
};

#define SYM_MAX 8
static char sym_buf[SYM_MAX];
static uint8_t sym_len = 0;

#define OUTQ_SIZE 16
static char outq[OUTQ_SIZE];
static uint8_t qh = 0, qt = 0;

static uint32_t Tu = 150; // default

static void qpush(char c)
{
    uint8_t next = (uint8_t)((qh + 1) % OUTQ_SIZE);
    if (next != qt) { // not full
        outq[qh] = c;
        qh = next;
    }
}

static uint8_t qpop(char *c)
{
    if (qt == qh) return 0;
    *c = outq[qt];
    qt = (uint8_t)((qt + 1) % OUTQ_SIZE);
    return 1;
}

static char decode_symbol(const char *code)
{
    for (unsigned i = 0; i < sizeof(table)/sizeof(table[0]); i++)
    {
        if (strcmp(table[i].code, code) == 0)
            return table[i].ch;
    }
    return '?';
}

void Morse_Init(uint32_t tu_ms)
{
    Tu = tu_ms;
    sym_len = 0;
    memset(sym_buf, 0, sizeof(sym_buf));
    qh = qt = 0;
}

uint32_t Morse_GetTu(void)
{
    return Tu;
}

void Morse_OnPulse(uint32_t dur_on_ms)
{
    if (sym_len >= (SYM_MAX - 1)) return;

    if (dur_on_ms < (2 * Tu))
        sym_buf[sym_len++] = '.';
    else
        sym_buf[sym_len++] = '-';

    sym_buf[sym_len] = '\0';
}

void Morse_OffGap(uint32_t dur_off_ms)
{
    // <1.5Tu -> separador interno: no hacemos nada
    if (dur_off_ms < (uint32_t)(1.5f * Tu))
        return;

    // 1.5 - 4.5 Tu -> fin de letra
    if (dur_off_ms < (uint32_t)(4.5f * Tu))
    {
        if (sym_len > 0)
        {
            char c = decode_symbol(sym_buf);
            qpush(c);
            sym_len = 0;
            sym_buf[0] = '\0';
        }
        return;
    }

    // >=6.5 Tu -> fin de palabra
    if (dur_off_ms >= (uint32_t)(6.5f * Tu))
    {
        // si había una letra pendiente, cerrala
        if (sym_len > 0)
        {
            char c = decode_symbol(sym_buf);
            qpush(c);
            sym_len = 0;
            sym_buf[0] = '\0';
        }
        qpush(' ');
    }
}

uint8_t Morse_PopChar(char *out)
{
    return qpop(out);
}


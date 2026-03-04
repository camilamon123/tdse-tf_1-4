/*
 * morse_decoder.h
 *
 *  Created on: Jan 21, 2026
 *      Author: feder
 */

#ifndef INC_APP_MORSE_DECODER_H_
#define INC_APP_MORSE_DECODER_H_

#pragma once
#include <stdint.h>

void Morse_Init(uint32_t tu_ms);

// Llamar cuando termina un ON y tenés la duración del pulso ON
void Morse_OnPulse(uint32_t dur_on_ms);

// Llamar cuando termina un OFF y tenés la duración del silencio OFF
void Morse_OffGap(uint32_t dur_off_ms);

// Saca chars decodificados (si hay)
uint8_t Morse_PopChar(char *out);

// Para que main conozca 'Tu' y use timeouts
uint32_t Morse_GetTu(void);

#endif /* INC_APP_MORSE_DECODER_H_ */
